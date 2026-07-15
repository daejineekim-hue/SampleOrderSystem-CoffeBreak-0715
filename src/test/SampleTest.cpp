#include <gtest/gtest.h>

#include "model/Sample.h"

// docs/FEATURES/sample-management.md 의 필드 검증 규칙(Acceptance Criteria)을
// 그대로 옮긴 테스트. Sample::validateFields()는 유효하지 않은 값에 대해
// std::invalid_argument를 던진다.

using sos::model::Sample;

namespace {

Sample makeValidSample() {
    Sample sample;
    sample.id = "S001";
    sample.name = "Wafer-A";
    sample.avgProcessTimeMin = 30;
    sample.yieldRate = 0.9;
    sample.stock = 100;
    return sample;
}

}  // namespace

TEST(SampleValidation, AcceptsValidSample) {
    Sample sample = makeValidSample();
    EXPECT_NO_THROW(Sample::validateFields(sample));
}

TEST(SampleValidation, RejectsEmptyId) {
    Sample sample = makeValidSample();
    sample.id = "";
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsEmptyName) {
    Sample sample = makeValidSample();
    sample.name = "";
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsZeroYieldRate) {
    Sample sample = makeValidSample();
    sample.yieldRate = 0;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsNegativeYieldRate) {
    Sample sample = makeValidSample();
    sample.yieldRate = -0.1;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsYieldRateAboveOne) {
    Sample sample = makeValidSample();
    sample.yieldRate = 1.1;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, AcceptsYieldRateExactlyOne) {
    Sample sample = makeValidSample();
    sample.yieldRate = 1.0;
    EXPECT_NO_THROW(Sample::validateFields(sample));
}

TEST(SampleValidation, RejectsZeroAvgProcessTime) {
    Sample sample = makeValidSample();
    sample.avgProcessTimeMin = 0;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsNegativeAvgProcessTime) {
    Sample sample = makeValidSample();
    sample.avgProcessTimeMin = -10;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, RejectsNegativeStock) {
    Sample sample = makeValidSample();
    sample.stock = -1;
    EXPECT_THROW(Sample::validateFields(sample), std::invalid_argument);
}

TEST(SampleValidation, AcceptsStockExactlyZero) {
    Sample sample = makeValidSample();
    sample.stock = 0;
    EXPECT_NO_THROW(Sample::validateFields(sample));
}
