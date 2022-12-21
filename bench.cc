#include <algorithm>
#include <functional>
#include <map>
#include <random>
#include <span>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "benchmark/benchmark.h"
#include "flat_hash_map/bytell_hash_map.hpp"
#include "hwy/contrib/sort/vqsort.h"
#include "tbb/enumerable_thread_specific.h"
#include "tbb/parallel_sort.h"

using int128 = __int128;

namespace std {
template <>
struct hash<int128> {
  size_t operator()(int128 v) const { return static_cast<uint64_t>(v); }
};
}  // namespace std

// using T = int64_t;
using T = int128;

const std::vector<T>& generate(int64_t domain, size_t size) {
  static std::map<std::pair<int64_t, size_t>, std::vector<T>> cache;
  if (auto it = cache.find({domain, size}); it != cache.end()) {
    return it->second;
  }
  std::mt19937 rng(0);
  std::uniform_int_distribution<int64_t> dist(0, domain);
  std::vector<T> data(size);
  for (T& datum : data) {
    datum = dist(rng);
  }
  return cache[{domain, size}] = std::move(data);
}

struct StdSort {
  constexpr static bool copy = true;
  template <typename T>
  void operator()(T* data, size_t size) {
    return std::sort(data, data + size);
  }
};

struct TbbSort {
  constexpr static bool copy = true;
  template <typename T>
  void operator()(T* data, size_t size) {
    return tbb::parallel_sort(data, data + size);
  }
};

template <typename T>
void InvokeHwySort(hwy::Sorter* sorter, T* data, size_t size) {
  (*sorter)(
      (std::conditional_t<std::is_same_v<T, int128>, hwy::uint128_t, T>*)data,
      size, hwy::SortAscending());
}

struct HwySort {
  constexpr static bool copy = true;
  hwy::Sorter sorter;
  template <typename T>
  void operator()(T* data, size_t size) {
    InvokeHwySort(&sorter, data, size);
  }
};

static constexpr auto kHashFn = [](auto v) {
  using T = std::decay_t<decltype(v)>;
  if constexpr (std::is_same_v<T, int128>) {
    return absl::Hash<std::pair<uint64_t, uint64_t>>{}(
        {static_cast<uint64_t>(v >> 64), static_cast<uint64_t>(v)});
  } else {
    return absl::Hash<T>{}(v);
  }
};

struct AbslHash {
  constexpr static bool copy = false;

  template <typename T>
  void operator()(const T* data, size_t size) {
    absl::flat_hash_set<T, decltype(kHashFn)> set;
    for (T datum : std::span(data, data + size)) {
      set.insert(datum);
    }
  }
};

struct BytellHash {
  constexpr static bool copy = false;
  template <typename T>
  void operator()(const T* data, size_t size) {
    ska::bytell_hash_set<T> set;
    for (T datum : std::span(data, data + size)) {
      set.insert(datum);
    }
  }
};

struct StdPartitionHwySort {
  constexpr static bool copy = true;
  tbb::enumerable_thread_specific<hwy::Sorter> tls;

  template <typename T>
  void operator()(T* data, size_t size) {
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
      InvokeHwySort(&tls.local(), begin, end - begin);
    };
    tg.run_and_wait(std::bind(task, data, data + size));
    tg.wait();
  }
};

struct JustCopy {
  constexpr static bool copy = true;
  template <typename T>
  void operator()(T* data, size_t size) {}
};


template <typename F>
static void BM_Sort(benchmark::State& state) {
  F sort;
  const auto& data = generate(state.range(1), state.range(0));
  std::unique_ptr<T[]> copy{new T[data.size()]};
  for (auto _ : state) {
    if constexpr (!sort.copy) {
      sort(data.data(), data.size());
    } else {
      state.PauseTiming();
      tbb::blocked_range<size_t> range(0, data.size(), 1 << 20);
      tbb::parallel_for(range, [&](decltype(range) r) {
        std::copy(&data[r.begin()], &data[r.end()], &copy[r.begin()]);
      });
      state.ResumeTiming();
      sort(copy.get(), data.size());
    }
  }
  state.SetItemsProcessed(state.iterations() * data.size());
  state.SetBytesProcessed(state.iterations() * data.size() * sizeof(data[0]));
}

#define BENCH(impl)                     \
  BENCHMARK(BM_Sort<impl>)              \
      ->Unit(benchmark::kMillisecond)   \
      ->Args({1'000'000'000, 1u << 30}) \
      ->MeasureProcessCPUTime();

BENCH(JustCopy);
BENCH(StdSort);
BENCH(AbslHash);
BENCH(BytellHash);
BENCH(TbbSort);
BENCH(HwySort);
BENCH(StdPartitionHwySort);
