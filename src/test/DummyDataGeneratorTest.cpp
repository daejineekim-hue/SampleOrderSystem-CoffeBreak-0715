#include <gtest/gtest.h>

#include <cstdio>

#include "generator/DummyDataGenerator.h"
#include "model/Sample.h"
#include "repository/SampleRepository.h"

// PLAN.md Phase 6 — demo/test seed data generation, ported from the
// DummyDataGenerator PoC.

using sos::generator::DummyDataGenerator;
using sos::model::Sample;
using sos::repository::SampleRepository;

namespace {

constexpr const char* kSampleFile = "dummy_data_generator_test_samples.json";

class DummyDataGeneratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::remove(kSampleFile);
        repo_ = std::make_unique<SampleRepository>(kSampleFile);
    }

    void TearDown() override { std::remove(kSampleFile); }

    std::unique_ptr<SampleRepository> repo_;
};

}  // namespace

TEST_F(DummyDataGeneratorTest, GeneratesRequestedCount) {
    DummyDataGenerator generator(*repo_);

    auto created = generator.generate(5);

    EXPECT_EQ(created.size(), 5u);
}

TEST_F(DummyDataGeneratorTest, GeneratedSamplesPassFieldValidation) {
    DummyDataGenerator generator(*repo_);

    auto created = generator.generate(10);

    for (const auto& sample : created) {
        EXPECT_NO_THROW(Sample::validateFields(sample)) << "sample id=" << sample.id;
    }
}

TEST_F(DummyDataGeneratorTest, GeneratedSamplesArePersisted) {
    DummyDataGenerator generator(*repo_);

    generator.generate(3);

    EXPECT_EQ(repo_->findAll().size(), 3u);
}

TEST_F(DummyDataGeneratorTest, DoesNotOverwriteExistingSamples) {
    Sample existing;
    existing.id = "SMP-001";
    existing.name = "Wafer-A";
    existing.avgProcessTimeMin = 10;
    existing.yieldRate = 0.9;
    existing.stock = 50;
    repo_->create(existing);

    DummyDataGenerator generator(*repo_);
    generator.generate(3);

    auto all = repo_->findAll();
    EXPECT_EQ(all.size(), 4u);
    bool foundExisting = false;
    for (const auto& sample : all) {
        if (sample.id == "SMP-001") {
            foundExisting = true;
            EXPECT_EQ(sample.stock, 50);
        }
    }
    EXPECT_TRUE(foundExisting);
}

TEST_F(DummyDataGeneratorTest, IdsAreUniqueAndContinueFromExistingMaxSuffix) {
    Sample existing;
    existing.id = "DUMMY-005";
    existing.name = "Existing";
    existing.avgProcessTimeMin = 10;
    existing.yieldRate = 0.9;
    existing.stock = 10;
    repo_->create(existing);

    DummyDataGenerator generator(*repo_);
    auto created = generator.generate(2);

    ASSERT_EQ(created.size(), 2u);
    EXPECT_EQ(created[0].id, "DUMMY-006");
    EXPECT_EQ(created[1].id, "DUMMY-007");
}

TEST_F(DummyDataGeneratorTest, GenerateZero_ReturnsEmpty) {
    DummyDataGenerator generator(*repo_);

    auto created = generator.generate(0);

    EXPECT_TRUE(created.empty());
    EXPECT_TRUE(repo_->findAll().empty());
}
