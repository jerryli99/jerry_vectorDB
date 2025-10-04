## A quick check of using SIMD vs not using SIMD
```
SIMD Support Check:
AVX: Supported
AVX2: Supported
FMA: Supported

=== Benchmark: 10000 vectors × 784 dimensions ===
Remainder elements: 0

Verification (tolerance: 0.1% relative or 0.001 absolute):
AVX Cleanup vs Naive: PASS
AVX Masked vs Naive: PASS
AVX Unrolled vs Naive: PASS

Performance Results:
Naive scalar:        13.845 ms
AVX + cleanup:       7.890 ms (speedup: 1.75x)
AVX + masked:        2.699 ms (speedup: 5.13x)
AVX + unrolled:      2.297 ms (speedup: 6.03x)

Performance Metrics (GFlops):
Naive:    1.13 GFlops
Best AVX: 6.83 GFlops
Fastest method: AVX + unrolled (6.02824x speedup)

============================================================

=== Benchmark: 10000 vectors × 787 dimensions ===
Remainder elements: 3

Verification (tolerance: 0.1% relative or 0.001 absolute):
AVX Cleanup vs Naive: PASS
AVX Masked vs Naive: PASS
AVX Unrolled vs Naive: PASS

Performance Results:
Naive scalar:        8.716 ms
AVX + cleanup:       2.804 ms (speedup: 3.11x)
AVX + masked:        2.835 ms (speedup: 3.07x)
AVX + unrolled:      2.848 ms (speedup: 3.06x)

Performance Metrics (GFlops):
Naive:    1.81 GFlops
Best AVX: 5.61 GFlops
Fastest method: AVX + cleanup (3.1082x speedup)

============================================================

=== Benchmark: 10000 vectors × 123 dimensions ===
Remainder elements: 3

Verification (tolerance: 0.1% relative or 0.001 absolute):
AVX Cleanup vs Naive: PASS
AVX Masked vs Naive: PASS
AVX Unrolled vs Naive: PASS

Performance Results:
Naive scalar:        1.272 ms
AVX + cleanup:       0.322 ms (speedup: 3.95x)
AVX + masked:        0.171 ms (speedup: 7.43x)
AVX + unrolled:      0.150 ms (speedup: 8.50x)

Performance Metrics (GFlops):
Naive:    1.93 GFlops
Best AVX: 16.43 GFlops
Fastest method: AVX + unrolled (8.50016x speedup)

============================================================

=== Benchmark: 10000 vectors × 257 dimensions ===
Remainder elements: 1

Verification (tolerance: 0.1% relative or 0.001 absolute):
AVX Cleanup vs Naive: PASS
AVX Masked vs Naive: PASS
AVX Unrolled vs Naive: PASS

Performance Results:
Naive scalar:        3.539 ms
AVX + cleanup:       1.054 ms (speedup: 3.36x)
AVX + masked:        0.950 ms (speedup: 3.73x)
AVX + unrolled:      0.820 ms (speedup: 4.32x)

Performance Metrics (GFlops):
Naive:    1.45 GFlops
Best AVX: 6.27 GFlops
Fastest method: AVX + unrolled (4.31714x speedup)

============================================================

=== Benchmark: 10000 vectors × 1000 dimensions ===
Remainder elements: 0

Verification (tolerance: 0.1% relative or 0.001 absolute):
AVX Cleanup vs Naive: PASS
AVX Masked vs Naive: PASS
AVX Unrolled vs Naive: PASS

Performance Results:
Naive scalar:        11.291 ms
AVX + cleanup:       4.552 ms (speedup: 2.48x)
AVX + masked:        2.732 ms (speedup: 4.13x)
AVX + unrolled:      2.843 ms (speedup: 3.97x)

Performance Metrics (GFlops):
Naive:    1.77 GFlops
Best AVX: 7.32 GFlops
Fastest method: AVX + masked (4.13271x speedup)

============================================================
```