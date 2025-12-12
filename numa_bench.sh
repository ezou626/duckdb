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

# Start perf (Intel NUMA events)
echo "[*] Starting perf..."
perf stat -e \
  mem_load_uops_retired.local_dram, \
  mem_load_uops_retired.remote_dram, \
  offcore_response.demand_data_rd.l3_hit.local_dram, \
  offcore_response.demand_data_rd.l3_hit.remote_dram \
  -p $BENCHPID \
  2> "$OUTDIR/perf_numa.log" &
PERFPID=$!

# Start numastat
echo "[*] Starting numastat..."
numastat -p $BENCHPID > "$OUTDIR/numastat.log" &
NUMAPID=$!

# Wait for benchmark to exit
echo "[*] Waiting for DuckDB benchmark to finish..."
wait $BENCHPID

# Cleanup
echo "[*] Benchmark complete. Killing perf + numastat..."
kill $PERFPID 2>/dev/null || true
kill $NUMAPID 2>/dev/null || true

echo "[*] Done! Results in: $OUTDIR/"
echo "    - perf_numa.log"
echo "    - numastat.log"
echo "    - timings.log"
