#include <vector>

#include "glog/logging.h"
#include "hwy/contrib/sort/vqsort.h"
#include "pdqsort.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_sort.h"

struct StdSort {
  template <typename T>
  void operator()(std::vector<T> v) {
    return std::sort(v.begin(), v.end());
  }
};

struct TbbSort {
  template <typename T>
  void operator()(std::vector<T> v) {
    return tbb::parallel_sort(v.begin(), v.end());
  }
};

struct HwySort {
  hwy::Sorter sorter;
  template <typename T>
  void operator()(std::vector<T> v) {
    using hwy_type = std::conditional_t<sizeof(T) == 16, hwy::uint128_t, T>;
    return sorter(reinterpret_cast<hwy_type*>(v.data()), v.size(), hwy::SortAscending());
  }
};

struct PdqSort {
  template <typename T>
  void operator()(std::vector<T> data) {
    pdqsort(data.begin(), data.end());
  }
};

struct StdPartitionHwySort {
  size_t grain_size = 16 << 20;

  tbb::enumerable_thread_specific<hwy::Sorter> tls;
  template <typename T>
  std::vector<T> operator()(std::vector<T> v) {
    tbb::task_group tg;
    std::function<void(T*, T*)> task;
    task = [&](T* begin, T* end) {
      while ((end - begin) > grain_size) {
        auto m = std::partition(begin, end - 1,
                                [pivot = end[-1]](T v) { return v < pivot; });
        std::swap(*m, end[-1]);
        tg.run(std::bind(task, m + 1, end));
        end = m;
      }
      using hwy_type = std::conditional_t<sizeof(T) == 16, hwy::uint128_t, T>;
      tls.local()(reinterpret_cast<hwy_type*>(begin), end - begin, hwy::SortAscending());
    };
    tg.run_and_wait(std::bind(task, v.data(), v.data() + v.size()));
    tg.wait();
    return std::move(v);
  }
};

#ifdef ENABLE_HWY_PARTITION_SORT
struct HwyPartitionHwySort {
  static constexpr int64_t GRAIN_SIZE = 16 << 20;

  template <typename T>
  void operator()(std::vector<T> v) {
    tbb::enumerable_thread_specific<hwy::Sorter> tls;
    tbb::task_group tg;
    std::function<void(T*, T*)> task;
    std::mutex mu;
    int64_t sort_calls = 0;
    int64_t total_sorted = 0;
    std::vector<std::pair<int64_t, int64_t>> calls;
    task = [&] (T*begin, T* end) {
      hwy::Sorter& sorter = tls.local();
      while ((end-begin) > GRAIN_SIZE) {
        T* split = begin + sorter.Partition(begin, end - begin, hwy::SortAscending());
        if (split == end) return;  // All equal -- done.
        tg.run(std::bind(task, split, end));
        end = split;
      }
      sorter(begin, end - begin, hwy::SortAscending());
      std::scoped_lock l(mu);
      total_sorted += end - begin;
      ++sort_calls;
      calls.emplace_back(begin - v.data(), end - v.data());
    };
    tg.run_and_wait(std::bind(task, v.data(), v.data() + v.size()));
    tg.wait();
    std::cerr << "sort_calls=" << sort_calls << " total_sorted=" << total_sorted << " n=" << v.size() << std::endl;
    std::sort(calls.begin(), calls.end());
    for (auto& [begin, end] : calls) {
      std::cerr << "  begin=" << begin  << " end=" << end << " size=" << (end-begin) << std::endl;
    }
    CHECK(std::is_sorted(v.begin(), v.end()));
  }
};
#endif

#ifdef ENABLE_INTEL_X86_SIMD_SORT
#include "intel-x86-simd-sort/avx512-16bit-qsort.hpp"
#include "intel-x86-simd-sort/avx512-32bit-qsort.hpp"
#include "intel-x86-simd-sort/avx512-64bit-qsort.hpp"
struct IntelX86SIMDSort {
  template <typename T>
  std::vector<T> operator()(std::vector<T> v) {
    avx512_qsort(v.data(), v.size());
    return v;
  }
};

#endif
