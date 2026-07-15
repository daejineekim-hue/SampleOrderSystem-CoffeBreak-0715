#include <gtest/gtest.h>

// Proves the GoogleTest pipeline (vendored source, project references,
// MSBuild/vstest) is wired correctly before any real feature work begins.
TEST(SmokeTest, GTestPipelineWorks) {
    EXPECT_EQ(1 + 1, 2);
}
