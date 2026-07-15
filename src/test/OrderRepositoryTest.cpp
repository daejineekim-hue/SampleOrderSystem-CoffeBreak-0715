#include <gtest/gtest.h>

#include <cstdio>

#include "model/Order.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

// docs/FEATURES/order-intake.md AC-1..AC-9.

using sos::model::Order;
using sos::model::OrderStatus;
using sos::repository::OrderRepository;
using sos::repository::SampleRepository;

namespace {

constexpr const char* kSampleFile = "order_intake_test_samples.json";
constexpr const char* kOrderFile = "order_intake_test_orders.json";

// A fixed, arbitrary instant used as "today" so date-dependent tests
// (order id format, per-day sequence) are reproducible regardless of when
// the test suite actually runs.
std::chrono::system_clock::time_point FixedNow() {
    std::tm tmv{};
    tmv.tm_year = 2026 - 1900;
    tmv.tm_mon = 3;  // April (0-indexed)
    tmv.tm_mday = 16;
    tmv.tm_hour = 9;
    tmv.tm_min = 32;
    tmv.tm_sec = 15;
    tmv.tm_isdst = -1;
    std::time_t t = std::mktime(&tmv);
    return std::chrono::system_clock::from_time_t(t);
}

std::chrono::system_clock::time_point OneDayBefore(std::chrono::system_clock::time_point tp) {
    return tp - std::chrono::hours(24);
}

class OrderRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
        sampleRepo_ = std::make_unique<SampleRepository>(kSampleFile);

        sos::model::Sample sample;
        sample.id = "SMP-001";
        sample.name = "Wafer-A";
        sample.avgProcessTimeMin = 30;
        sample.yieldRate = 0.9;
        sample.stock = 100;
        sampleRepo_->create(sample);
    }

    void TearDown() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
    }

    OrderRepository makeOrderRepo(OrderRepository::Clock clock = &FixedNow) {
        return OrderRepository(kOrderFile, *sampleRepo_, std::move(clock));
    }

    std::unique_ptr<SampleRepository> sampleRepo_;
};

}  // namespace

TEST_F(OrderRepositoryTest, RegistersReservedOrderWithGeneratedId) {
    OrderRepository repo = makeOrderRepo();

    const Order& order = repo.registerOrder("SMP-001", "홍길동", 10);

    EXPECT_EQ(order.status, OrderStatus::RESERVED);
    EXPECT_EQ(order.orderId, "ORD-20260416-0001");
    EXPECT_EQ(order.sampleId, "SMP-001");
    EXPECT_EQ(order.customerName, "홍길동");
    EXPECT_EQ(order.quantity, 10);
    EXPECT_FALSE(order.createdAt.empty());
}

TEST_F(OrderRepositoryTest, SequenceIncrementsWithinSameDay) {
    OrderRepository repo = makeOrderRepo();
    repo.registerOrder("SMP-001", "A", 1);
    repo.registerOrder("SMP-001", "B", 1);

    const Order& third = repo.registerOrder("SMP-001", "C", 1);

    EXPECT_EQ(third.orderId, "ORD-20260416-0003");
}

TEST_F(OrderRepositoryTest, RejectsNonExistentSampleId) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-999", "홍길동", 10), std::invalid_argument);
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(OrderRepositoryTest, RejectsZeroQuantity) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-001", "홍길동", 0), std::invalid_argument);
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(OrderRepositoryTest, RejectsNegativeQuantity) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-001", "홍길동", -5), std::invalid_argument);
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(OrderRepositoryTest, RejectsEmptyCustomerName) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-001", "", 10), std::invalid_argument);
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(OrderRepositoryTest, RejectsBlankCustomerName) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-001", "   ", 10), std::invalid_argument);
    EXPECT_TRUE(repo.findAll().empty());
}

TEST_F(OrderRepositoryTest, SequenceResetsOnNewDay) {
    auto yesterday = OneDayBefore(FixedNow());
    bool useYesterday = true;
    OrderRepository::Clock clock = [&]() { return useYesterday ? yesterday : FixedNow(); };
    OrderRepository repo = makeOrderRepo(clock);

    for (int i = 0; i < 7; ++i) {
        repo.registerOrder("SMP-001", "A", 1);
    }
    useYesterday = false;

    const Order& firstToday = repo.registerOrder("SMP-001", "B", 1);

    EXPECT_EQ(firstToday.orderId, "ORD-20260416-0001");
}

TEST_F(OrderRepositoryTest, FailedValidationDoesNotConsumeSequence) {
    OrderRepository repo = makeOrderRepo();

    EXPECT_THROW(repo.registerOrder("SMP-999", "홍길동", 0), std::invalid_argument);
    const Order& order = repo.registerOrder("SMP-001", "홍길동", 10);

    EXPECT_EQ(order.orderId, "ORD-20260416-0001");
}

TEST_F(OrderRepositoryTest, PersistsAcrossRepositoryInstances) {
    {
        OrderRepository repo = makeOrderRepo();
        repo.registerOrder("SMP-001", "홍길동", 10);
    }
    OrderRepository reopened = makeOrderRepo();

    ASSERT_EQ(reopened.findAll().size(), 1u);
    EXPECT_EQ(reopened.findAll()[0].orderId, "ORD-20260416-0001");
}
