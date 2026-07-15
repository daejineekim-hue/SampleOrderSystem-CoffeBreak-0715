#include <gtest/gtest.h>

#include <cstdio>

#include "repository/SampleRepository.h"

// docs/FEATURES/sample-management.md 의 시료 등록/조회/검색 Acceptance Criteria를
// SampleRepository의 CRUD API에 대한 테스트로 옮긴 것.

using sos::model::Sample;
using sos::repository::SampleRepository;

namespace {

class SampleRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override { std::remove(TestFilePath()); }
    void TearDown() override { std::remove(TestFilePath()); }

    static const char* TestFilePath() { return "sample_repository_test.json"; }

    SampleRepository makeRepo() { return SampleRepository(TestFilePath()); }

    Sample makeSample(const std::string& id, const std::string& name, int stock = 0) {
        Sample sample;
        sample.id = id;
        sample.name = name;
        sample.avgProcessTimeMin = 30;
        sample.yieldRate = 0.9;
        sample.stock = stock;
        return sample;
    }
};

}  // namespace

TEST_F(SampleRepositoryTest, RegistersValidSampleAndItAppearsInList) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A", 100));

    auto all = repo.findAll();
    ASSERT_EQ(all.size(), 1u);
    EXPECT_EQ(all[0].id, "S001");
    EXPECT_EQ(all[0].name, "Wafer-A");
    EXPECT_EQ(all[0].stock, 100);
}

TEST_F(SampleRepositoryTest, StockDefaultsToZeroWhenOmitted) {
    SampleRepository repo = makeRepo();
    Sample sample = makeSample("S001", "Wafer-A");  // stock defaults to 0 above
    repo.create(sample);

    EXPECT_EQ(repo.findAll()[0].stock, 0);
}

TEST_F(SampleRepositoryTest, RejectsDuplicateId) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    EXPECT_THROW(repo.create(makeSample("S001", "Different-Name")), std::invalid_argument);
    EXPECT_EQ(repo.findAll()[0].name, "Wafer-A");  // unchanged
}

TEST_F(SampleRepositoryTest, ListReturnsEmptyWhenNoSamplesRegistered) {
    SampleRepository repo = makeRepo();
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(SampleRepositoryTest, ListReflectsLatestStock) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A", 50));

    ASSERT_TRUE(repo.updateStock("S001", 30));

    EXPECT_EQ(repo.findAll()[0].stock, 30);
}

TEST_F(SampleRepositoryTest, SearchMatchesExactName) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    auto results = repo.search("Wafer-A");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S001");
}

TEST_F(SampleRepositoryTest, SearchMatchesSubstring) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    auto results = repo.search("afer");

    ASSERT_EQ(results.size(), 1u);
}

TEST_F(SampleRepositoryTest, SearchReturnsAllMatchesAndExcludesNonMatches) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));
    repo.create(makeSample("S002", "Wafer-B"));
    repo.create(makeSample("S003", "Chip-C"));

    auto results = repo.search("Wafer");

    ASSERT_EQ(results.size(), 2u);
}

TEST_F(SampleRepositoryTest, SearchIsCaseSensitive) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    EXPECT_TRUE(repo.search("WAFER").empty());
}

TEST_F(SampleRepositoryTest, SearchReturnsEmptyWhenNoMatch) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    EXPECT_TRUE(repo.search("NonExistent").empty());
}

TEST_F(SampleRepositoryTest, SearchWithEmptyKeywordReturnsAllSamples) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));
    repo.create(makeSample("S002", "Wafer-B"));

    EXPECT_EQ(repo.search("").size(), 2u);
}

TEST_F(SampleRepositoryTest, SearchMatchesById) {
    SampleRepository repo = makeRepo();
    repo.create(makeSample("S001", "Wafer-A"));

    auto results = repo.search("S00");

    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].id, "S001");
}

TEST_F(SampleRepositoryTest, PersistsAcrossRepositoryInstances) {
    {
        SampleRepository repo = makeRepo();
        repo.create(makeSample("S001", "Wafer-A", 100));
    }
    SampleRepository reopened = makeRepo();
    ASSERT_EQ(reopened.findAll().size(), 1u);
    EXPECT_EQ(reopened.findAll()[0].id, "S001");
}
