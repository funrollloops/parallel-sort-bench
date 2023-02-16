Benchmarks of various sorting algorithms, including parallel sorts.


Benchmark results on a GCE n2-standard-8 instance (Ice Lake):
```
----------------------------------------------------------------------------------------------------------------------
Benchmark                                          Time             CPU   Iterations bytes_per_second items_per_second
----------------------------------------------------------------------------------------------------------------------
BM_Sort<TbbSort>/process_time                   25.9 s           143 s             1       55.9032M/s       7.32734M/s
BM_Sort<HwySort>/process_time                   19.5 s          19.5 s             1       410.459M/s       53.7997M/s
BM_Sort<IntelX86SIMDSort>/process_time          32.5 s          32.5 s             1       246.494M/s       32.3084M/s
BM_Sort<StdPartitionHwySort>/process_time       16.4 s          49.0 s             1       163.279M/s       21.4013M/s
```

Note that hwy outperforms tbb sort (which uses `std::sort`) with 8 threads!
