#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from gwosc_common import (
    DATA_ROOT,
    DETAILS_ROOT,
    PARAMETERS_ROOT,
    ensure_layout,
    fetch_json,
    read_json,
    slugify_event_name,
    write_json,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Fetch per-event detail and parameter metadata from GWOSC."
    )
    parser.add_argument(
        "--catalog-file",
        type=Path,
        default=DATA_ROOT / "catalog_events.json",
        help="Path to catalog JSON from step 01",
    )
    parser.add_argument(
        "--refresh",
        action="store_true",
        help="Re-fetch metadata even if cached",
    )
    parser.add_argument(
        "--limit",
        type=int,
        default=None,
        help="Only process the first N events from the catalog file",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Optional output path for the generated event index JSON",
    )
    return parser.parse_args()


def pick_preferred_pe_result(parameters_payload: dict) -> dict | None:
    pe_results = [
        item for item in parameters_payload.get("results", [])
        if item.get("pipeline_type") == "pe"
    ]
    if not pe_results:
        return None

    preferred = [item for item in pe_results if item.get("is_preferred")]
    return preferred[0] if preferred else pe_results[0]


def find_skymap_link(pe_result: dict | None) -> dict | None:
    if not pe_result:
        return None

    for link in pe_result.get("links", []):
        if link.get("label") == "skymap":
            return link
    return None


def load_or_fetch_json(url: str, cache_path: Path, refresh: bool) -> dict:
    if cache_path.exists() and not refresh:
        return read_json(cache_path)

    payload = fetch_json(url)
    write_json(cache_path, payload)
    return payload


def main() -> int:
    args = parse_args()
    ensure_layout()

    catalog_payload = read_json(args.catalog_file)
    events = catalog_payload.get("results", [])
    if args.limit is not None:
        events = events[:args.limit]

    output_path = args.output
    if output_path is None:
        if args.limit is None:
            output_path = DATA_ROOT / "event_index.json"
        else:
            output_path = DATA_ROOT / f"event_index.limit-{args.limit}.json"

    index = []
    for event in events:
        short_name = event["shortName"]
        slug = slugify_event_name(short_name)

        detail_path = DETAILS_ROOT / f"{slug}.json"
        detail_payload = load_or_fetch_json(event["detail_url"], detail_path, args.refresh)

        parameters_url = detail_payload.get(
            "parameters_url",
            f"https://gwosc.org/api/v2/event-versions/{short_name}/parameters?format=api",
        )
        parameters_path = PARAMETERS_ROOT / f"{slug}.json"
        parameters_payload = load_or_fetch_json(parameters_url, parameters_path, args.refresh)

        preferred_pe = pick_preferred_pe_result(parameters_payload)
        skymap_link = find_skymap_link(preferred_pe)

        index.append({
            "name": event["name"],
            "short_name": short_name,
            "catalog": event.get("catalog"),
            "gps": event.get("gps"),
            "detail_url": event.get("detail_url"),
            "parameters_url": parameters_url,
            "detail_cache": str(detail_path.relative_to(DATA_ROOT.parent)),
            "parameters_cache": str(parameters_path.relative_to(DATA_ROOT.parent)),
            "preferred_pe_name": preferred_pe.get("name") if preferred_pe else None,
            "preferred_pe_pipeline": preferred_pe.get("pipeline") if preferred_pe else None,
            "skymap_url": skymap_link.get("url") if skymap_link else None,
            "skymap_label": skymap_link.get("label") if skymap_link else None,
            "skymap_description": skymap_link.get("description") if skymap_link else None,
            "status": "has_skymap" if skymap_link else "missing_skymap",
        })
        print(
            f"{short_name}: "
            f"{'skymap found' if skymap_link else 'no skymap link'}"
        )

    write_json(output_path, {
        "catalog_file": str(args.catalog_file),
        "results_count": len(index),
        "results": index,
    })

    print()
    print(f"Wrote event index to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
