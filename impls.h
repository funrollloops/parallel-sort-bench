#include <vector>

#include "glog/logging.h"
#include "hwy/contrib/sort/vqsort.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_sort.h"

using T = hwy::uint128_t;

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
    return sorter(v.data(), v.size(), hwy::SortAscending());
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
      tls.local()(begin, end - begin, hwy::SortAscending());
    };
    tg.run_and_wait(std::bind(task, v.data(), v.data() + v.size()));
    tg.wait();
    return std::move(v);
  }
};
