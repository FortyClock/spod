#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
repository_root=$(cd "$script_dir/../.." && pwd)
binary=${SPOD_BENCH_BINARY:-"$repository_root/build/volleyball_benchmark"}
output=${1:-"$repository_root/pr2/results/volleyball_benchmark_raw.csv"}
passes=${SPOD_BENCH_PASSES:-500}
repetitions=${SPOD_BENCH_REPETITIONS:-7}
warmups=${SPOD_BENCH_WARMUPS:-2}
process_counts=${SPOD_BENCH_PROCESSES:-"1 2 4 8 16"}
payload_counts=${SPOD_BENCH_PAYLOADS:-"1 1024 65536"}

if [[ ! -x "$binary" ]]; then
    echo "Benchmark executable not found: $binary" >&2
    exit 1
fi

mkdir -p "$(dirname "$output")"
temporary_output=$(mktemp)
trap 'rm -f "$temporary_output"' EXIT

printf '%s\n' \
    'payload_ints,payload_bytes,processes,passes,run,time_seconds,mode' \
    >"$temporary_output"

for payload_count in $payload_counts; do
    for process_count in $process_counts; do
        echo "Measuring n=$payload_count, p=$process_count" >&2
        mpiexec \
            --use-hwthread-cpus \
            --oversubscribe \
            -n "$process_count" \
            "$binary" \
            "$payload_count" \
            "$passes" \
            "$repetitions" \
            "$warmups" \
            >>"$temporary_output"
    done
done

mv "$temporary_output" "$output"
chmod 644 "$output"
trap - EXIT
echo "Raw results: $output" >&2
