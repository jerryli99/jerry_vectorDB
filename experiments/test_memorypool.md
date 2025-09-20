## Benchmark Results

### Small Test (10,000 point objss)

| Test Case            | Time (μs) | Reallocations | Capacity |
|----------------------|-----------|---------------|----------|
| No pool              | 19,045    | 15            | 16,384   |
| No pool + reserve    | 20,566    | 0             | 10,000   |
| With memory pool     | 7,914     | 0             | 10,000   |
| Batch allocation     | 17,530    | 0             | 10,000   |

## Large Test (70,000 point objs)

| Test Case            | Time (μs) | Reallocations | Capacity |
|----------------------|-----------|---------------|----------|
| No pool              | 125,137   | 18            | 131,072  |
| No pool + reserve    | 64,380    | 0             | 70,000   |
| With memory pool     | 37,478    | 0             | 70,000   |
| Batch allocation     | 35,668    | 0             | 70,000   |

```
--- Memory Pool Test with Access ---

=== Small test ===
Testing ACCESS performance (2000 points)...
Access time without pool: 1856 μs
Access time with pool: 943 μs
Access speedup: 49.1918%
=== Large test ===
Testing ACCESS performance (70000 points)...
Access time without pool: 25081 μs
Access time with pool: 16986 μs
Access speedup: 32.2754%

--- Memory Pool Test with Access ---

=== Small test ===
Testing ACCESS performance (2000 points)...
Access time without pool: 628 μs
Access time with pool: 499 μs
Access speedup: 20.5414%
=== Large test ===
Testing ACCESS performance (70000 points)...
Access time without pool: 16563 μs
Access time with pool: 15351 μs
Access speedup: 7.31751%
```

OK, now another testing

```
=== Memory Pool Test with Reallocation Tracking ===

=== Small Test (2000 points) ===
Testing WITHOUT memory pool (2000 points)...
Time taken: 4208 microseconds
Reallocations: 12 times
Final capacity: 2048 points
Memory fragmentation: HIGH

Testing WITHOUT memory pool WITH reserve (2000 points)...
Time taken: 3437 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Memory fragmentation: MEDIUM

Testing WITH simple memory pool (2000 points)...
Time taken: 10077 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Pool usage: 0/2000 points

Testing FAST batch allocation (2000 points)...
Time taken: 4224 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Batch allocated: 2000 points

=== Large Test (70000 points) ===
Testing WITHOUT memory pool (70000 points)...
Time taken: 181045 microseconds
Reallocations: 18 times
Final capacity: 131072 points
Memory fragmentation: HIGH

Testing WITHOUT memory pool WITH reserve (70000 points)...
Time taken: 126101 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Memory fragmentation: MEDIUM

Testing WITH simple memory pool (70000 points)...
Time taken: 88642 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Pool usage: 0/70000 points

Testing FAST batch allocation (70000 points)...
Time taken: 91433 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Batch allocated: 70000 points



=== Memory Pool Test with Reallocation Tracking ===

=== Small Test (2000 points) ===
Testing WITHOUT memory pool (2000 points)...
Time taken: 3929 microseconds
Reallocations: 12 times
Final capacity: 2048 points
Memory fragmentation: HIGH

Testing WITHOUT memory pool WITH reserve (2000 points)...
Time taken: 3840 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Memory fragmentation: MEDIUM

Testing WITH simple memory pool (2000 points)...
Time taken: 4915 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Pool usage: 0/2000 points

Testing FAST batch allocation (2000 points)...
Time taken: 3924 microseconds
Reallocations: 0 times
Final capacity: 2000 points
Batch allocated: 2000 points

=== Large Test (70000 points) ===
Testing WITHOUT memory pool (70000 points)...
Time taken: 202294 microseconds
Reallocations: 18 times
Final capacity: 131072 points
Memory fragmentation: HIGH

Testing WITHOUT memory pool WITH reserve (70000 points)...
Time taken: 120279 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Memory fragmentation: MEDIUM

Testing WITH simple memory pool (70000 points)...
Time taken: 112523 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Pool usage: 0/70000 points

Testing FAST batch allocation (70000 points)...
Time taken: 92463 microseconds
Reallocations: 0 times
Final capacity: 70000 points
Batch allocated: 70000 points

```