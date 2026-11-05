#!/usr/bin/env python3

import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} RAW.csv SUMMARY.csv", file=sys.stderr)
        return 1

    raw_path = Path(sys.argv[1])
    summary_path = Path(sys.argv[2])
    measurements: dict[tuple[int, int, int, int, str], list[float]] = defaultdict(list)

    with raw_path.open(newline="", encoding="utf-8") as source:
        for row in csv.DictReader(source):
            key = (
                int(row["payload_ints"]),
                int(row["payload_bytes"]),
                int(row["processes"]),
                int(row["passes"]),
                row["mode"],
            )
            measurements[key].append(float(row["time_seconds"]))

    medians = {
        key: statistics.median(values)
        for key, values in measurements.items()
    }

    summary_path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames = [
        "payload_ints",
        "payload_bytes",
        "processes",
        "passes",
        "repetitions",
        "min_ms",
        "median_ms",
        "max_ms",
        "speedup",
        "efficiency",
        "mode",
    ]

    with summary_path.open("w", newline="", encoding="utf-8") as destination:
        writer = csv.DictWriter(
            destination,
            fieldnames=fieldnames,
            lineterminator="\n",
        )
        writer.writeheader()

        for key in sorted(measurements):
            payload_ints, payload_bytes, processes, passes, mode = key
            values = measurements[key]
            baseline_key = (payload_ints, payload_bytes, 1, passes, "local_copy")
            baseline = medians[baseline_key]
            median = medians[key]
            speedup = baseline / median
            efficiency = speedup / processes

            writer.writerow(
                {
                    "payload_ints": payload_ints,
                    "payload_bytes": payload_bytes,
                    "processes": processes,
                    "passes": passes,
                    "repetitions": len(values),
                    "min_ms": f"{min(values) * 1000:.6f}",
                    "median_ms": f"{median * 1000:.6f}",
                    "max_ms": f"{max(values) * 1000:.6f}",
                    "speedup": f"{speedup:.6f}",
                    "efficiency": f"{efficiency:.6f}",
                    "mode": mode,
                }
            )

    print(f"Summary: {summary_path}", file=sys.stderr)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
