#!/usr/bin/env python3
"""Run the existing fixed-size benchmark; save raw times and medians."""

import argparse
import csv
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import re
import shlex
import statistics
import subprocess
import sys
from datetime import datetime, timezone


ROOT = Path(__file__).resolve().parents[2]
TIME_PATTERN = re.compile(r"^Avg max time ([0-9.eE+-]+) seconds$", re.MULTILINE)


def positive_int(value):
    number = int(value)
    if number < 1:
        raise argparse.ArgumentTypeError("expected a positive integer")
    return number


def version(command):
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=10)
        return result.stdout.strip() or result.stderr.strip()
    except (OSError, subprocess.TimeoutExpired) as error:
        return str(error)


def write_latex_table(summaries, table_path):
    table_lines = [
        r"\begin{table}[H]",
        r"\caption{Время одной редукции: семь запусков без прогрева, мкс}",
        r"\label{tab:reduce-benchmark}",
        r"\centering",
        r"\begin{tabular}{cc|rrr}",
        r"$K$ & $n=K+5$ & Минимум & Медиана & Максимум \\",
        r"\hline",
    ]
    for row in summaries:
        times = [f"{float(value):.3f}".replace(".", ",") for value in row[4:7]]
        table_lines.append(" & ".join([str(row[0]), str(row[1]), *times]) + r" \\")
    table_lines.extend([r"\end{tabular}", r"\end{table}"])
    table_path.write_text("\n".join(table_lines) + "\n", encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=ROOT / "build/reduce_sum_benchmark")
    parser.add_argument("--processes", nargs="+", type=positive_int, default=[1, 2, 4, 8, 16])
    parser.add_argument("--repetitions", type=positive_int, default=7)
    parser.add_argument("--output-dir", type=Path, default=ROOT / "pr3/results")
    parser.add_argument("--launcher", default="mpirun")
    parser.add_argument("--oversubscribe", action="store_true",
                        help="allow more MPI processes than available slots")
    args = parser.parse_args()
    binary = args.binary.resolve()
    source = ROOT / "pr3/src/reduce_sum_benchmark.c"
    if not binary.is_file() or not os.access(binary, os.X_OK):
        parser.error(f"build the executable first: {binary}")
    if source.stat().st_mtime > binary.stat().st_mtime:
        parser.error("source is newer than executable; rebuild reduce_sum_benchmark")
    source_text = source.read_text(encoding="utf-8")
    # These constants describe the current exercise, not a configurable n.
    iterations = int(re.search(r"^#define COUNT_ITER\s+(\d+)\s*$", source_text, re.MULTILINE)[1])
    offset = int(re.search(r"^#define MAGIC_NUM\s+(\d+)\s*$", source_text, re.MULTILINE)[1])
    flags = ["--use-hwthread-cpus"]
    if args.oversubscribe:
        flags.append("--oversubscribe")
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=True)
    raw_path = output / "reduce_sum_benchmark_raw.csv"
    summary_path = output / "reduce_sum_benchmark_summary.csv"
    metadata_path = output / "reduce_sum_benchmark_metadata.json"
    metadata = {
        "started_utc": datetime.now(timezone.utc).isoformat(),
        "status": "running",
        "platform": platform.platform(),
        "logical_cpus": os.cpu_count(),
        "cpu_model": next((line.split(":", 1)[1].strip()
                           for line in Path("/proc/cpuinfo").read_text().splitlines()
                           if line.startswith("model name")), "unknown"),
        "binary": str(binary),
        "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
        "source_sha256": hashlib.sha256(source.read_bytes()).hexdigest(),
        "launcher_version": version([args.launcher, "--version"]),
        "launcher_flags": flags,
        "processes": list(dict.fromkeys(args.processes)),
        "repetitions": args.repetitions,
        "warmups": 0,
        "iterations_per_run": iterations,
        "array_length_rule": f"processes + {offset}",
        "timing": "maximum local series duration / iterations; each repeat starts a new MPI job",
        "commands": [],
    }
    cache_path = binary.parent / "CMakeCache.txt"
    if cache_path.is_file():
        cache_text = cache_path.read_text()
        for key in ("CMAKE_BUILD_TYPE", "CMAKE_C_COMPILER", "CMAKE_C_FLAGS",
                    "CMAKE_C_FLAGS_RELEASE"):
            match = re.search(rf"^{key}:[^=]+=(.*)$", cache_text, re.MULTILINE)
            if match:
                metadata[key] = match[1]
        if "CMAKE_C_COMPILER" in metadata:
            metadata["compiler_version"] = version([metadata["CMAKE_C_COMPILER"], "--version"])
    metadata_path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n")
    # A failed run must not leave a summary from a previous experiment.
    summary_path.unlink(missing_ok=True)
    summaries = []
    with raw_path.open("w", newline="", encoding="utf-8") as raw:
        writer = csv.writer(raw, lineterminator="\n")
        writer.writerow(["processes", "array_length", "iterations_per_run", "run",
                         "seconds_per_reduce", "oversubscribe_allowed"])
        for processes in metadata["processes"]:
            values = []
            command = [args.launcher, *flags, "-np", str(processes), str(binary)]
            metadata["commands"].append(shlex.join(command))
            for run in range(1, args.repetitions + 1):
                result = subprocess.run(command, capture_output=True, text=True, timeout=60)
                if result.returncode:
                    raise RuntimeError(f"p={processes}, run={run}:\n{result.stdout}\n{result.stderr}")
                matches = TIME_PATTERN.findall(result.stdout)
                if len(matches) != 1:
                    raise RuntimeError(f"expected one timing line, got:\n{result.stdout}")
                seconds = float(matches[0])
                if not math.isfinite(seconds) or seconds <= 0:
                    raise RuntimeError("time must be positive; check printf precision")
                values.append(seconds)
                writer.writerow([processes, processes + offset, iterations, run,
                                 f"{seconds:.12f}", int(args.oversubscribe)])
                raw.flush()
                print(f"p={processes:2d}, run={run}/{args.repetitions}: "
                      f"{seconds * 1e6:.6f} us/reduce", flush=True)
            summaries.append([processes, processes + offset, iterations, len(values),
                              f"{min(values) * 1e6:.6f}",
                              f"{statistics.median(values) * 1e6:.6f}",
                              f"{max(values) * 1e6:.6f}", int(args.oversubscribe)])
    with summary_path.open("w", newline="", encoding="utf-8") as summary:
        writer = csv.writer(summary, lineterminator="\n")
        writer.writerow(["processes", "array_length", "iterations_per_run", "repetitions",
                         "min_us", "median_us", "max_us", "oversubscribe_allowed"])
        writer.writerows(summaries)
    table_path = output / "reduce_sum_benchmark_table.tex"
    write_latex_table(summaries, table_path)
    metadata["status"] = "complete"
    metadata["finished_utc"] = datetime.now(timezone.utc).isoformat()
    metadata_path.write_text(json.dumps(metadata, ensure_ascii=False, indent=2) + "\n")
    print(f"\nRaw: {raw_path}\nSummary: {summary_path}\nMetadata: {metadata_path}"
          f"\nLaTeX table: {table_path}")


if __name__ == "__main__":
    try:
        main()
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        sys.exit(str(error))
