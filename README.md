Benchmarks of various sorting algorithms, including parallel sorts.


Benchmark results on a GCE n2-standard-8 instance (Ice Lake):
```
Run on (8 X 2600.02 MHz CPU s)
CPU Caches:
  L1 Data 48 KiB (x4)
  L1 Instruction 32 KiB (x4)
  L2 Unified 1280 KiB (x4)
  L3 Unified 55296 KiB (x1)
Load Average: 0.15, 0.38, 0.19
---------------------------------------------------------------------------------------------------------------------
Benchmark                                                     Time             CPU   Iterations      Bytes      Items
---------------------------------------------------------------------------------------------------------------------
BM_Sort/TbbSort/random/process_time                        3.17 s          16.7 s             1 61.2207M/s 8.02432M/s
BM_Sort/TbbSort/sorted_runs/process_time                   1.58 s          8.02 s             1 127.615M/s 16.7267M/s
BM_Sort/HwySort/random/process_time                        2.27 s          2.27 s             1 451.365M/s 59.1613M/s
BM_Sort/HwySort/sorted_runs/process_time                   2.38 s          2.38 s             1 429.987M/s 56.3593M/s
BM_Sort/StdPartitionHwySort/random/process_time            2.06 s          4.01 s             1 255.071M/s 33.4327M/s
BM_Sort/StdPartitionHwySort/sorted_runs/process_time       1.11 s          3.21 s             1 318.723M/s 41.7757M/s
BM_Sort/PdqSort/random/process_time                        5.67 s          5.67 s             1 180.528M/s 23.6622M/s
BM_Sort/PdqSort/sorted_runs/process_time                   5.30 s          5.30 s             1 193.329M/s   25.34M/s
BM_Sort/IntelX86SIMDSort/random/process_time               3.73 s          3.73 s             1 274.734M/s 36.0099M/s
BM_Sort/IntelX86SIMDSort/sorted_runs/process_time          2.90 s          2.90 s             1 352.558M/s 46.2105M/s
```

* `tbb::sort` is only able to utilize 5.2 of 8 cores, on average, because the early partition steps are so slow.
* `hwy::Sorter` is able to outperform `tbb::sort` on random data despite using only a single thread, but on sorted runs it does not go any faster.
* `std::partition` + `hwy::Sorter` to make a parallel algorithm only helps a little in wall time for random data, but makes a surprisingly large difference for sorted runs.
   note the pivot selection used here is just the last element, which is obviously not robust.
*  `pdqsort` does not benefit from sorted runs, oddly.
*  `avx512_sort` runs a bit faster with sorted runs, but it still slower than `hwy`. 64% and 22% slower for random and sorted runs, respectively.
