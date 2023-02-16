#include <random>

#include <gtest/gtest.h>
#include <gmock/gmock-matchers.h>

#include "impls.h"

using ::testing::ElementsAreArray;

TEST(Sort, StdPartitionHwySort) {
  std::vector<int32_t> data(1 << 20);
  std::mt19937 rng;
  std::uniform_int_distribution<int32_t> dist;
  for (int32_t& datum : data) datum = dist(rng);

  StdPartitionHwySort sorter{.grain_size = 1024}; 
  std::vector<int32_t> sorted = sorter(static_cast<const std::vector<int32_t>>(data));
  ASSERT_TRUE(std::is_sorted(sorted.begin(), sorted.end()));

  std::sort(data.begin(), data.end());
  EXPECT_THAT(sorted, ElementsAreArray(data));
}
