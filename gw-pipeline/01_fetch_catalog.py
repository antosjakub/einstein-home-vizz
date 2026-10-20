#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from gwosc_common import DATA_ROOT, ensure_layout, fetch_json, write_json


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Fetch the paginated GWOSC event catalog metadata."
    )
    parser.add_argument("--catalog", default="GWTC", help="Catalog name, default: GWTC")
    parser.add_argument(
        "--release",
        default=None,
        help="Comma-separated release filter for /event-versions, for example GWTC-1-confident,GWTC-2.1-confident,GWTC-3-confident,GWTC-4.0",
    )
    parser.add_argument("--pagesize", type=int, default=100, help="API page size")
    parser.add_argument(
        "--lastver",
        action="store_true",
        help="Keep only the latest version of each event",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    ensure_layout()

    if args.release:
        base_url = (
            "https://gwosc.org/api/v2/event-versions"
            f"?format=api&pagesize={args.pagesize}&release={args.release}"
        )
        if args.lastver:
            base_url += "&lastver=true"
    else:
        base_url = (
            f"https://gwosc.org/api/v2/catalogs/{args.catalog}/events"
            f"?format=api&pagesize={args.pagesize}"
        )

    results = []
    page_urls = []
    next_url = base_url

    while next_url:
        payload = fetch_json(next_url)
        page_urls.append(next_url)
        results.extend(payload.get("results", []))
        next_url = payload.get("next")
        print(
            f"Fetched page {payload.get('page_number')} / {payload.get('num_pages')}: "
            f"{len(payload.get('results', []))} events"
        )

    if args.lastver:
        by_name = {}
        for item in results:
            existing = by_name.get(item["name"])
            if existing is None or item.get("version", 0) > existing.get("version", 0):
                by_name[item["name"]] = item
        results = sorted(by_name.values(), key=lambda item: item["gps"], reverse=True)
        print()
        print(f"Reduced to latest versions only: {len(results)} events")

    output = {
        "catalog": args.catalog,
        "release": args.release,
        "pagesize": args.pagesize,
        "lastver": args.lastver,
        "page_urls": page_urls,
        "results_count": len(results),
        "results": results,
    }

    output_path = DATA_ROOT / "catalog_events.json"
    write_json(output_path, output)

    print()
    print(f"Wrote {len(results)} events to {output_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
