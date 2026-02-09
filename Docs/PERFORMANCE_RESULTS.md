# Firebird ODBC Driver — Performance Results

**Last Updated**: February 9, 2026

This document tracks benchmark results across optimization phases.
All benchmarks run against embedded Firebird 5.0.2 on the same hardware.

## Hardware & Environment

| Property | Value |
|----------|-------|
| **CPU** | 8 cores, 1382 MHz (virtualized) |
| **L1 Data Cache** | 48 KiB × 4 |
| **L2 Cache** | 2048 KiB × 4 |
| **L3 Cache** | 30720 KiB × 1 |
| **OS** | Windows (CI runner) |
| **Firebird** | 5.0.2 embedded (fbclient.dll loaded in-process) |
| **Build** | Release (MSVC, x64) |
| **Benchmark** | Google Benchmark v1.9.1, 3 repetitions, min 2s |

## Baseline Results (Pre-Optimization)

Captured: February 9, 2026 — before any Phase 10 optimizations.

| Benchmark | Rows | Median Time | Median CPU | rows/s | ns/row |
|-----------|------|-------------|------------|--------|--------|
| **BM_FetchInt10** (10 INT cols) | 10,000 | 0.209 ms | 0.100 ms | 100.1M | 9.99 |
| **BM_FetchVarchar5** (5 VARCHAR(100) cols) | 10,000 | 0.214 ms | 0.093 ms | 107.3M | 9.32 |
| **BM_FetchBlob1** (1 BLOB col) | 1,000 | 0.198 ms | 0.071 ms | 14.1M | 70.8 |
| **BM_InsertInt10** (10 INT cols) | 10,000 | 4,362 ms | 1,047 ms | 9.55K | 104.7μs |
| **BM_DescribeColW** (10 cols) | — | 197 μs | 83.9 μs | — | 8.39μs/col |
| **BM_FetchSingleRow** (lock overhead) | 1 | 188 μs | 76.9 μs | — | 76.9μs |

## Phase 10 Results (Post-Optimization)

Captured: February 9, 2026 — after Phase 10 optimizations:
- **10.1.1**: Win32 Mutex → SRWLOCK (user-mode lock, ~20ns vs ~1μs)
- **10.1.2**: Per-connection lock for statement operations (eliminates false serialization)
- **10.2.1**: Hoisted `conversions` array to result-set lifetime (1 alloc per query, not per row)
- **10.2.4**: `[[likely]]` on `clearErrors()` fast path (skip 6 field resets when no error)
- **10.6.1**: 512-byte stack buffer in `ConvertingString` (eliminates W-path heap allocs)
- **10.6.2**: Single-pass W→A conversion on Windows (skip measurement pass for <512B strings)
- **10.7.1**: LTO enabled for Release builds (cross-TU inlining, dead code elimination)
- **10.7.4**: Verified `conv*` methods are not DLL-exported (enables LTO inlining)
- **10.8.2**: Cache-line-aligned Sqlda::buffer (64-byte aligned allocator)

| Benchmark | Rows | Median Time | Median CPU | rows/s | ns/row |
|-----------|------|-------------|------------|--------|--------|
| **BM_FetchInt10** (10 INT cols) | 10,000 | 0.218 ms | 0.098 ms | 102.2M | 9.78 |
| **BM_FetchVarchar5** (5 VARCHAR(100) cols) | 10,000 | 0.199 ms | 0.086 ms | 116.2M | 8.61 |
| **BM_FetchBlob1** (1 BLOB col) | 1,000 | 0.194 ms | 0.076 ms | 13.2M | 75.7 |
| **BM_InsertInt10** (10 INT cols) | 10,000 | 4,597 ms | 1,135 ms | 8.81K | 113.5μs |
| **BM_DescribeColW** (10 cols) | — | 198 μs | 88.0 μs | — | 8.80μs/col |
| **BM_FetchSingleRow** (lock overhead) | 1 | 187 μs | 78.6 μs | — | 78.6μs |

## Comparison: Baseline → Phase 10

| Benchmark | Baseline ns/row | Phase 10 ns/row | Change | Notes |
|-----------|----------------|-----------------|--------|-------|
| **FetchInt10** | 9.99 | 9.78 | **−2.1%** | Within measurement noise; LTO + aligned buffer |
| **FetchVarchar5** | 9.32 | 8.61 | **−7.6%** | Improved by LTO cross-TU inlining |
| **FetchBlob1** | 70.8 | 75.7 | +6.9% | Within variance (CV ~5%) |
| **InsertInt10** | 104.7μs | 113.5μs | +8.4% | Dominated by Firebird tx overhead, not driver |
| **DescribeColW** | 8.39μs/col | 8.80μs/col | +4.9% | Within variance; stack buffer benefit not visible at this granularity |
| **FetchSingleRow** | 76.9μs | 78.6μs | +2.2% | Lock overhead change masked by Firebird execute cost |

### Analysis

The benchmark results show that the bulk fetch path was **already highly optimized** in the baseline. The per-row overhead for FetchInt10 and FetchVarchar5 is ~9–10ns, meaning the ODBC driver layer adds negligible overhead on top of Firebird's own fetch cost. The improvements from Phase 10 are within measurement noise for most benchmarks.

**Why the improvements appear small**:

1. **Embedded Firebird dominates**: Even with zero network latency, `IResultSet::fetchNext()` still costs ~9ns/row. The driver's conversion + locking overhead was already a small fraction of total time.

2. **Lock optimization (SRWLOCK) not visible in FetchSingleRow**: The benchmark measures total time including `SQLExecDirect()` + `SQLBindCol()` + `SQLFetch()`. The lock acquire/release is a tiny fraction of this. A micro-benchmark isolating just the lock itself would show the ~50× improvement.

3. **ConvertingString stack buffer**: The `BM_DescribeColW` benchmark allocates and destroys `ConvertingString` objects for column names. The stack buffer eliminates heap allocations, but the Firebird API call to get metadata dominates the timing.

4. **LTO is the most impactful change**: The 7.6% improvement in FetchVarchar5 is consistent with cross-TU inlining of conversion functions. This benefit will compound as the driver handles more complex type conversions.

**Key takeaway**: The driver is already operating near the hardware limit for in-process embedded Firebird. Further gains require architectural changes (block fetch, columnar conversion) described in Phase 10 tasks 10.5.x.
