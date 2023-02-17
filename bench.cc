#include <algorithm>
#include <functional>
#include <map>
#include <random>

#include "benchmark/benchmark.h"
#include "impls.h"

namespace {
using T = uint64_t;
static constexpr size_t kSize = 1 << 30;
static constexpr size_t kBlockSize = 1 << 20;

std::vector<T> random() {
  std::vector<T> data(kSize);
  tbb::parallel_for(0UL, kSize / kBlockSize, [&](int shard) {
    std::mt19937 rng(shard);
    std::uniform_int_distribution<uint32_t> dist;
    for (int i = shard * kBlockSize, end = i + kBlockSize; i != end; ++i) {
      data[i] = dist(rng);
    }
  });
  return data;
}

std::vector<T> sorted_runs() {
  std::vector<T> data(kSize);
  tbb::parallel_for(0UL, kSize / kBlockSize, [&](int shard) {
    std::mt19937 rng(shard);
    std::uniform_int_distribution<uint8_t> dist8(0, 127);
    int i = shard * kBlockSize;
    data[i] = std::uniform_int_distribution<uint32_t>()(rng);
    for (int end = i + kBlockSize; i != end; ++i) {
      data[i] = data[i - 1] + dist8(rng);
    }
  });
  return data;
}

template <typename Sorter>
static void BM_Sort(benchmark::State& state, Sorter sorter,
                    std::vector<T> (*gen)()) {
  const auto& data = gen();
  for (auto _ : state) {
    sorter(data);  // Implicit copy.
  }
  using ::benchmark::Counter;
  state.counters["Items"] = Counter(kSize, Counter::kIsIterationInvariantRate);
  state.counters["Bytes"] = Counter(kSize*sizeof(T), Counter::kIsIterationInvariantRate, Counter::OneK::kIs1024);
}

#define BENCH2(impl, dist)                            \
  BENCHMARK_CAPTURE(BM_Sort, impl/dist, impl{}, dist) \
      ->Unit(benchmark::kSecond)                      \
      ->MeasureProcessCPUTime();

#define BENCH(impl)     \
  BENCH2(impl, random); \
  BENCH2(impl, sorted_runs);

BENCH(TbbSort);
BENCH(HwySort);
BENCH(StdPartitionHwySort);
BENCH(PdqSort);
#if ENABLE_HWY_PARTITION_SORT
BENCH(HwyPartitionHwySort);
#endif
#if ENABLE_INTEL_X86_SIMD_SORT
BENCH(IntelX86SIMDSort);
#endif
}  // namespace

int main(int argc, char* argv[]) {
  google::InitGoogleLogging(argv[0]);
  benchmark::Initialize(&argc, argv);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InstallFailureSignalHandler();
  benchmark::RunSpecifiedBenchmarks();
  benchmark::Shutdown();
}
