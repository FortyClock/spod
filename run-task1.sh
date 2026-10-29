#!/usr/bin/env bash

set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
cd "$script_dir"

# Values can be overridden without editing the script, for example:
# PROCESS_COUNTS="2 4" BUFFER_SIZES="1000 100000" ./run-task1.sh
read -r -a process_counts <<< "${PROCESS_COUNTS:-1 2 4 8 16}"
read -r -a buffer_sizes <<< "${BUFFER_SIZES:-1 1000 100000 1000000}"
warmup_runs=${WARMUP_RUNS:-2}
measured_runs=${MEASURED_RUNS:-8}

if (( measured_runs < 1 )); then
    echo "MEASURED_RUNS must be at least 1" >&2
    exit 1
fi

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target task1_benchmark

timestamp=$(date +%Y%m%d_%H%M%S)
result_dir="pr1/results"
raw_csv="$result_dir/task1_benchmark_raw_${timestamp}.csv"
summary_csv="$result_dir/task1_benchmark_summary_${timestamp}.csv"
mkdir -p "$result_dir"

printf 'task,processes,elements,bytes,run,time_seconds\n' > "$raw_csv"
printf 'task,processes,elements,bytes,repetitions,median_seconds\n' > "$summary_csv"

median()
{
    local values=("$@")
    local sorted
    local count
    local middle

    mapfile -t sorted < <(printf '%s\n' "${values[@]}" | LC_ALL=C sort -g)
    count=${#sorted[@]}
    middle=$((count / 2))

    if (( count % 2 == 1 )); then
        printf '%s' "${sorted[$middle]}"
    else
        awk -v left="${sorted[$((middle - 1))]}" -v right="${sorted[$middle]}" \
            'BEGIN { printf "%.9f", (left + right) / 2.0 }'
    fi
}

run_once()
{
    local processes=$1
    local elements=$2
    local -a mpi_options=(--use-hwthread-cpus)
    local output

    if (( processes > $(nproc) )); then
        mpi_options+=(--oversubscribe)
    fi

    output=$(mpirun "${mpi_options[@]}" -np "$processes" \
        ./build/task1_benchmark "$elements")

    if [[ $output =~ time=([0-9]+([.][0-9]+)?) ]]; then
        printf '%s' "${BASH_REMATCH[1]}"
    else
        echo "Could not read time from program output: $output" >&2
        return 1
    fi
}

total_combinations=$((${#process_counts[@]} * ${#buffer_sizes[@]}))
combination=0

for processes in "${process_counts[@]}"; do
    for elements in "${buffer_sizes[@]}"; do
        combination=$((combination + 1))
        bytes=$((elements * 4))

        printf '[%d/%d] processes=%d elements=%d (%d bytes)\n' \
            "$combination" "$total_combinations" "$processes" "$elements" "$bytes"

        for ((run = 1; run <= warmup_runs; run++)); do
            printf '  warm-up %d/%d\r' "$run" "$warmup_runs"
            run_once "$processes" "$elements" > /dev/null
        done
        if (( warmup_runs > 0 )); then
            printf '%*s\r' 40 ''
        fi

        times=()
        for ((run = 1; run <= measured_runs; run++)); do
            elapsed=$(run_once "$processes" "$elements")
            times+=("$elapsed")
            printf '  run %d/%d: %s s\n' "$run" "$measured_runs" "$elapsed"
            printf 'task1,%d,%d,%d,%d,%s\n' \
                "$processes" "$elements" "$bytes" "$run" "$elapsed" >> "$raw_csv"
        done

        median_time=$(median "${times[@]}")
        printf 'task1,%d,%d,%d,%d,%s\n' \
            "$processes" "$elements" "$bytes" "$measured_runs" "$median_time" \
            >> "$summary_csv"
        printf '  median: %s s\n\n' "$median_time"
    done
done

echo "Raw measurements: $raw_csv"
echo "Median summary:   $summary_csv"
