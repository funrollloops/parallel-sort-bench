#include <random>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "impls.h"

using ::testing::ElementsAreArray;

template <typename T>
std::vector<T> generate_random_data(size_t N) {
  std::vector<T> data(1 << 20);
  std::mt19937 rng;
  std::uniform_int_distribution<int32_t> dist;
  for (T& datum : data) datum = dist(rng);
  return data;
}

TEST(Sort, StdPartitionHwySort) {
  auto data = generate_random_data<int32_t>(1 << 20);
  StdPartitionHwySort sorter{.grain_size = 1024}; 
  std::vector<int32_t> sorted = sorter(static_cast<const std::vector<int32_t>>(data));
  ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
  std::sort(data.begin(), data.end());
  EXPECT_THAT(sorted, ElementsAreArray(data));
}

#if ENABLE_INTEL_X86_SIMD_SORT
TEST(Sort, IntelX86SIMDSort) {
  auto data = generate_random_data<int32_t>(1 << 20);
  IntelX86SIMDSort sorter;
  std::vector<int32_t> sorted = sorter.template operator()<int32_t>(std::vector(data));
  ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));
  std::sort(data.begin(), data.end());
  EXPECT_THAT(sorted, ElementsAreArray(data));
}
#endif
