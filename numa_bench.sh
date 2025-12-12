#!/usr/bin/env bash
set -euo pipefail

# Path to benchmark runner
RUNNER=./build/release/benchmark/benchmark_runner

# Allow regex or single benchmark path
if [ $# -lt 1 ]; then
    echo "Usage: $0 <benchmark_regex_or_path>"
    echo "Example: $0 'benchmark/micro/nulls/.*'"
    exit 1
fi

BENCHMARK_EXPR="$1"

# Output directory
OUTDIR=run_numa_$(date +%s)
mkdir -p "$OUTDIR"

# Start DuckDB benchmark runner
echo "[*] Starting DuckDB benchmark runner..."
$RUNNER "$BENCHMARK_EXPR" --out="$OUTDIR/timings.log" &
BENCHPID=$!

echo "[*] Benchmark PID = $BENCHPID"

# Start numastat sampler (1 Hz)
echo "[*] Starting numastat sampler..."
(
  # Print CSV header once
  echo "timestamp,metric,node0,node1,node2,node3"

  while kill -0 "$BENCHPID" 2>/dev/null; do
      TS=$(date +%s)

      # Run numastat -p
      numastat -p "$BENCHPID" | \
      awk -v ts="$TS" '
        /^[A-Za-z]/ {
            name=$1
            gsub(":", "", name)
            printf "%s,%s",$0 == "" ? "" : ts, name
            for (i=2;i<=NF;i++) printf ",%s", $i
            printf "\n"
        }
      '

      sleep 1
  done
) > "$OUTDIR/numastat.csv" &
NUMAPID=$!

# Wait for benchmark to exit
echo "[*] Waiting for DuckDB benchmark to finish..."
wait "$BENCHPID"

# Cleanup
echo "[*] Benchmark complete. Killing numastat..."
kill "$NUMAPID" 2>/dev/null || true

echo "[*] Done! Results in: $OUTDIR/"
echo "    - numastat.csv"
echo "    - timings.log"