#include <algorithm>
#include <functional>
#include <map>
#include <random>

#include "benchmark/benchmark.h"

#include "impls.h"

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

template <typename F>
static void BM_Sort(benchmark::State& state) {
  F sort;
  const auto& data = generate(10000L << 20, 1000 << 20);
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
BENCH(IntelX86SIMDSort);
BENCH(StdPartitionHwySort);
