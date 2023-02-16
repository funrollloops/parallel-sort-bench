#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <vector>

#include "benchmark/benchmark.h"
#include "glog/logging.h"
#include "hwy/contrib/sort/vqsort.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_sort.h"

using T = hwy::uint128_t;

const std::vector<T>& generate(uint64_t domain, size_t size) {
  static std::map<std::pair<uint64_t, size_t>, std::vector<T>> cache;
  if (auto found = cache.find({domain, size}); found != cache.end()) {
    return found->second;
  }
  std::mt19937 rng(0);
  std::uniform_int_distribution<uint64_t> dist(0, domain);
  std::vector<T> data(size);
  for (T& datum : data) {
    datum = T{dist(rng)};
  }
  return cache[{domain, size}] = std::move(data);
}
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
  tbb::enumerable_thread_specific<hwy::Sorter> tls;
  template <typename T>
  void operator()(std::vector<T> v) {
    tbb::task_group tg;
    std::function<void(T*, T*)> task;
    task = [&](T* begin, T* end) {
      while ((end - begin) > 16 << 20) {
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
    CHECK(std::is_sorted(v.begin(), v.end()));
  }
};

template <typename F>
static void BM_Sort(benchmark::State& state) {
  F sort;
  const auto data = generate(10000L << 20, 1000 << 20);
  for (auto _ : state) {
    sort(data);  // Implicit copy.
  }
  state.SetItemsProcessed(state.iterations() * data.size());
  state.SetBytesProcessed(state.iterations() * data.size() * sizeof(T));
}

#define BENCH(impl)                 \
  BENCHMARK(BM_Sort<impl>) \
      ->Unit(benchmark::kSecond)    \
      ->MeasureProcessCPUTime();

BENCH(StdSort);
BENCH(TbbSort);
BENCH(HwySort);
BENCH(StdPartitionHwySort);
