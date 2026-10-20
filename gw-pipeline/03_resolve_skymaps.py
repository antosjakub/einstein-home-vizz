#!/usr/bin/env python3

from __future__ import annotations

import argparse
import gzip
import io
import math
import tarfile
from pathlib import Path

import numpy as np

from gwosc_common import (
    DATA_ROOT,
    RESOLVED_ROOT,
    SKYMAPS_ROOT,
    download_binary,
    ensure_layout,
    read_json,
    slugify_event_name,
    url_digest,
    write_json,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Download missing skymaps and resolve each event to one RA/Dec marker."
    )
    parser.add_argument(
        "--event-index",
        type=Path,
        default=DATA_ROOT / "event_index.json",
        help="Path to the event index from step 02",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Recompute coordinates even if already resolved",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Only resolve the first N entries from the event index",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional output path for the resolved source JSON",
    )
    return parser.parse_args()


def require_optional_dependencies() -> tuple[object, object, object]:
    try:
        from astropy.table import Table
        from astropy_healpix import healpix_to_lonlat, uniq_to_level_ipix
    except ImportError as exc:
        raise SystemExit(
            "Step 03 requires 'astropy' and 'astropy-healpix'.\n"
            "Install them with:\n"
            "  python3 -m pip install astropy astropy-healpix\n"
            f"Original import error: {exc}"
        ) from exc

    return Table, healpix_to_lonlat, uniq_to_level_ipix


def choose_cache_path(url: str) -> Path:
    name = url.split("/")[-2] if url.endswith("/content") else url.split("/")[-1]
    name = name.split("?")[0]
    if not name:
        name = "download"
    return SKYMAPS_ROOT / f"{url_digest(url)}-{name}"


def download_if_needed(url: str, refresh: bool) -> Path:
    destination = choose_cache_path(url)
    if destination.exists() and not refresh:
        return destination

    print(f"Downloading skymap archive: {url}")
    return download_binary(url, destination)


def looks_like_fits(name: str) -> bool:
    lowered = name.lower()
    return (
        lowered.endswith(".fits")
        or lowered.endswith(".fit")
        or lowered.endswith(".fits.gz")
        or lowered.endswith(".fit.gz")
        or lowered.endswith(".multiorder.fits")
        or lowered.endswith(".multiorder.fits.gz")
    )


def pick_tar_member(archive_path: Path, event_name: str, short_name: str) -> str:
    with tarfile.open(archive_path, "r:*") as archive:
        members = [member for member in archive.getmembers() if member.isfile() and looks_like_fits(member.name)]

    if not members:
        raise RuntimeError(f"No FITS members found in {archive_path}")

    event_tokens = {
        event_name.lower(),
        short_name.lower(),
        short_name.split("-v", 1)[0].lower(),
    }

    def rank(member_name: str) -> tuple[int, int, str]:
        lowered = member_name.lower()
        token_score = 0 if any(token in lowered for token in event_tokens) else 1
        ext_score = 0 if ".multiorder.fits" in lowered else 1
        return (token_score, ext_score, lowered)

    ranked = sorted((member.name for member in members), key=rank)
    return ranked[0]


def load_fits_table_bytes(archive_path: Path, member_name: str | None) -> bytes:
    if member_name is None:
        return archive_path.read_bytes()

    with tarfile.open(archive_path, "r:*") as archive:
        member = archive.getmember(member_name)
        handle = archive.extractfile(member)
        if handle is None:
            raise RuntimeError(f"Failed to extract {member_name} from {archive_path}")
        return handle.read()


def open_fits_as_table(table_class: object, payload: bytes):
    buffer = io.BytesIO(payload)
    if payload[:2] == b"\x1f\x8b":
        buffer = io.BytesIO(gzip.decompress(payload))
    return table_class.read(buffer, format="fits")


def resolve_standard_healpix(table, healpix_to_lonlat) -> tuple[float, float, float]:
    probs = np.asarray(table["PROB"], dtype=float)
    index = int(np.argmax(probs))

    nside = int(table.meta["NSIDE"])
    ordering = str(table.meta.get("ORDERING", "NESTED")).strip().lower()
    order = "nested" if ordering.startswith("nest") else "ring"

    lon, lat = healpix_to_lonlat(index, nside, order=order)
    return float(lon.deg), float(lat.deg), float(probs[index])


def resolve_multiorder_healpix(table, healpix_to_lonlat, uniq_to_level_ipix) -> tuple[float, float, float]:
    densities = np.asarray(table["PROBDENSITY"], dtype=float)
    index = int(np.argmax(densities))

    uniq = int(table["UNIQ"][index])
    level, ipix = uniq_to_level_ipix(uniq)
    nside = 2 ** int(level)

    lon, lat = healpix_to_lonlat(int(ipix), nside, order="nested")
    pixel_area_sr = math.pi / (3.0 * nside * nside)
    probability_mass = float(densities[index] * pixel_area_sr)
    return float(lon.deg), float(lat.deg), probability_mass


def resolve_table(table, healpix_to_lonlat, uniq_to_level_ipix) -> tuple[float, float, float]:
    columns = set(table.colnames)
    if {"UNIQ", "PROBDENSITY"}.issubset(columns):
        return resolve_multiorder_healpix(table, healpix_to_lonlat, uniq_to_level_ipix)
    if "PROB" in columns and "NSIDE" in table.meta:
        return resolve_standard_healpix(table, healpix_to_lonlat)
    raise RuntimeError(f"Unsupported skymap layout with columns: {table.colnames}")


def resolve_event(entry: dict, table_class, healpix_to_lonlat, uniq_to_level_ipix, refresh: bool) -> dict:
    short_name = entry["short_name"]
    resolved_path = RESOLVED_ROOT / f"{slugify_event_name(short_name)}.json"
    if resolved_path.exists() and not refresh:
        return read_json(resolved_path)

    skymap_url = entry.get("skymap_url")
    if not skymap_url:
        result = {
            "name": entry["name"],
            "short_name": short_name,
            "catalog": entry.get("catalog"),
            "gps": entry.get("gps"),
            "status": "missing_skymap",
        }
        write_json(resolved_path, result)
        return result

    archive_path = download_if_needed(skymap_url, refresh)
    member_name = None
    if tarfile.is_tarfile(archive_path):
        member_name = pick_tar_member(archive_path, entry["name"], short_name)

    payload = load_fits_table_bytes(archive_path, member_name)
    table = open_fits_as_table(table_class, payload)
    ra_deg, dec_deg, probability_proxy = resolve_table(
        table,
        healpix_to_lonlat,
        uniq_to_level_ipix,
    )

    result = {
        "name": entry["name"],
        "short_name": short_name,
        "catalog": entry.get("catalog"),
        "gps": entry.get("gps"),
        "status": "resolved",
        "ra_deg": ra_deg,
        "dec_deg": dec_deg,
        "probability_proxy": probability_proxy,
        "skymap_url": skymap_url,
        "archive_path": str(archive_path.relative_to(DATA_ROOT.parent)),
        "skymap_member": member_name,
    }
    write_json(resolved_path, result)
    return result


def main() -> int:
    args = parse_args()
    ensure_layout()
    table_class, healpix_to_lonlat, uniq_to_level_ipix = require_optional_dependencies()

    event_index = read_json(args.event_index)
    results = []
    output_path = args.output
    if output_path is None:
        if args.limit is None:
            output_path = DATA_ROOT / "resolved_sources.json"
        else:
            output_path = DATA_ROOT / f"resolved_sources.limit-{args.limit}.json"

    entries = event_index.get("results", [])
    if args.limit is not None:
        entries = entries[:args.limit]

    for entry in entries:
        result = resolve_event(
            entry,
            table_class,
            healpix_to_lonlat,
            uniq_to_level_ipix,
            args.refresh,
        )
        results.append(result)
        print(f"{result['short_name']}: {result['status']}")

    write_json(output_path, {
        "event_index": str(args.event_index),
        "results_count": len(results),
        "results": results,
    })

    print()
    print(f"Wrote resolved source list to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
