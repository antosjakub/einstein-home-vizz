#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
from collections import Counter
from pathlib import Path

from gwosc_common import DATA_ROOT, read_json


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export resolved GW merge source positions to a readable CSV."
    )
    parser.add_argument(
        "--resolved-file",
        type=Path,
        default=DATA_ROOT / "resolved_sources.json",
        help="Path to the resolved source JSON from step 03",
    )
    return parser.parse_args()


def write_csv(resolved_results: list[dict], output_path: Path) -> None:
    with output_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "event",
            "short_name",
            "catalog",
            "gps",
            "ra_deg",
            "dec_deg",
            "probability_proxy",
            "skymap_member",
        ])
        for item in resolved_results:
            writer.writerow([
                item["name"],
                item["short_name"],
                item.get("catalog"),
                item.get("gps"),
                f"{item['ra_deg']:.6f}",
                f"{item['dec_deg']:.6f}",
                f"{item.get('probability_proxy', 0.0):.12g}",
                item.get("skymap_member") or "",
            ])


def write_report(all_results: list[dict], csv_results: list[dict], output_path: Path) -> None:
    counts = Counter(item.get("status", "unknown") for item in all_results)
    lines = [
        "# Merge Sources Export",
        "",
        f"- total events: {len(all_results)}",
        f"- resolved markers: {len(csv_results)}",
    ]
    for status, count in sorted(counts.items()):
        lines.append(f"- {status}: {count}")

    if csv_results:
        lines.extend([
            "",
            "## First resolved entries",
            "",
            "| event | ra_deg | dec_deg |",
            "| --- | ---: | ---: |",
        ])
        for item in csv_results[:10]:
            lines.append(
                f"| {item['short_name']} | {item['ra_deg']:.4f} | {item['dec_deg']:.4f} |"
            )

    output_path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> int:
    args = parse_args()
    payload = read_json(args.resolved_file)
    all_results = payload.get("results", [])
    csv_results = [item for item in all_results if item.get("status") == "resolved"]

    csv_path = DATA_ROOT / "merge_sources.csv"
    report_path = DATA_ROOT / "merge_sources_report.md"

    write_csv(csv_results, csv_path)
    write_report(all_results, csv_results, report_path)

    print(f"Wrote CSV: {csv_path}")
    print(f"Wrote report: {report_path}")
    print(f"Resolved markers exported: {len(csv_results)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
