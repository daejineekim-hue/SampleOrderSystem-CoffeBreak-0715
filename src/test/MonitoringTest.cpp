#include <gtest/gtest.h>

#include "model/Monitoring.h"
#include "model/Order.h"
#include "model/Sample.h"

// docs/FEATURES/monitoring.md Acceptance Criteria.

using sos::model::Monitoring;
using sos::model::Order;
using sos::model::OrderStatus;
using sos::model::Sample;
using sos::model::StockLevel;

namespace {

Order makeOrder(const std::string& sampleId, OrderStatus status, int quantity) {
    Order o;
    o.orderId = "ORD-" + sampleId + "-" + std::to_string(quantity);
    o.sampleId = sampleId;
    o.customerName = "customer";
    o.quantity = quantity;
    o.status = status;
    return o;
}

Sample makeSample(const std::string& id, int stock) {
    Sample s;
    s.id = id;
    s.name = id;
    s.avgProcessTimeMin = 10;
    s.yieldRate = 0.9;
    s.stock = stock;
    return s;
}

}  // namespace

// ---- 주문량 확인 (Order Count by Status) ----

TEST(MonitoringCountByStatusTest, CountsEachStatusCorrectly) {
    std::vector<Order> orders = {
        makeOrder("S001", OrderStatus::RESERVED, 1), makeOrder("S001", OrderStatus::RESERVED, 1),
        makeOrder("S001", OrderStatus::CONFIRMED, 1), makeOrder("S001", OrderStatus::PRODUCING, 1),
        makeOrder("S001", OrderStatus::PRODUCING, 1), makeOrder("S001", OrderStatus::PRODUCING, 1),
        makeOrder("S001", OrderStatus::RELEASE, 1),
    };

    auto counts = Monitoring::countByStatus(orders);

    EXPECT_EQ(counts.reserved, 2);
    EXPECT_EQ(counts.confirmed, 1);
    EXPECT_EQ(counts.producing, 3);
    EXPECT_EQ(counts.release, 1);
}

TEST(MonitoringCountByStatusTest, ExcludesRejected) {
    std::vector<Order> orders = {makeOrder("S001", OrderStatus::RESERVED, 1),
                                  makeOrder("S001", OrderStatus::REJECTED, 1),
                                  makeOrder("S001", OrderStatus::REJECTED, 1),
                                  makeOrder("S001", OrderStatus::REJECTED, 1),
                                  makeOrder("S001", OrderStatus::REJECTED, 1),
                                  makeOrder("S001", OrderStatus::REJECTED, 1)};

    auto counts = Monitoring::countByStatus(orders);

    EXPECT_EQ(counts.reserved, 1);
    EXPECT_EQ(counts.confirmed + counts.producing + counts.release, 0);
}

TEST(MonitoringCountByStatusTest, ZeroForMissingStatus) {
    std::vector<Order> orders = {makeOrder("S001", OrderStatus::RESERVED, 1),
                                  makeOrder("S001", OrderStatus::RESERVED, 1),
                                  makeOrder("S001", OrderStatus::RESERVED, 1)};

    auto counts = Monitoring::countByStatus(orders);

    EXPECT_EQ(counts.reserved, 3);
    EXPECT_EQ(counts.confirmed, 0);
    EXPECT_EQ(counts.producing, 0);
    EXPECT_EQ(counts.release, 0);
}

TEST(MonitoringCountByStatusTest, AllZeroWhenNoOrders) {
    auto counts = Monitoring::countByStatus({});

    EXPECT_EQ(counts.reserved, 0);
    EXPECT_EQ(counts.confirmed, 0);
    EXPECT_EQ(counts.producing, 0);
    EXPECT_EQ(counts.release, 0);
}

TEST(MonitoringCountByStatusTest, SumsAcrossSamples) {
    std::vector<Order> orders = {makeOrder("S001", OrderStatus::RESERVED, 1),
                                  makeOrder("S002", OrderStatus::RESERVED, 1)};

    auto counts = Monitoring::countByStatus(orders);

    EXPECT_EQ(counts.reserved, 2);
}

// ---- 재고량 확인 (Stock Status by Sample) ----

TEST(MonitoringStockStatusTest, Sufficient_NoPendingDemand) {
    std::vector<Sample> samples = {makeSample("S001", 480)};

    auto statuses = Monitoring::stockStatus(samples, {});

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, Low_StockLessThanPendingDemand) {
    std::vector<Sample> samples = {makeSample("S002", 30)};
    std::vector<Order> orders = {makeOrder("S002", OrderStatus::RESERVED, 100),
                                  makeOrder("S002", OrderStatus::PRODUCING, 70)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 170);
    EXPECT_EQ(statuses[0].level, StockLevel::Low);
}

TEST(MonitoringStockStatusTest, Sufficient_StockCoversPendingDemand) {
    std::vector<Sample> samples = {makeSample("S003", 200)};
    std::vector<Order> orders = {makeOrder("S003", OrderStatus::RESERVED, 50),
                                  makeOrder("S003", OrderStatus::PRODUCING, 100)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 150);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, Boundary_StockEqualsPendingDemand_Sufficient) {
    std::vector<Sample> samples = {makeSample("S004", 150)};
    std::vector<Order> orders = {makeOrder("S004", OrderStatus::RESERVED, 150)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, Depleted_ZeroStockNoPendingDemand) {
    std::vector<Sample> samples = {makeSample("S005", 0)};

    auto statuses = Monitoring::stockStatus(samples, {});

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].level, StockLevel::Depleted);
}

TEST(MonitoringStockStatusTest, Depleted_ZeroStockTakesPriorityOverPendingDemand) {
    std::vector<Sample> samples = {makeSample("S006", 0)};
    std::vector<Order> orders = {makeOrder("S006", OrderStatus::RESERVED, 50)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].level, StockLevel::Depleted);
}

TEST(MonitoringStockStatusTest, ConfirmedAndRelease_ExcludedFromPendingDemand) {
    std::vector<Sample> samples = {makeSample("S007", 10)};
    std::vector<Order> orders = {makeOrder("S007", OrderStatus::CONFIRMED, 500),
                                  makeOrder("S007", OrderStatus::RELEASE, 300)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 0);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, Rejected_ExcludedFromPendingDemand) {
    std::vector<Sample> samples = {makeSample("S008", 5)};
    std::vector<Order> orders = {makeOrder("S008", OrderStatus::REJECTED, 1000)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 0);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, SampleWithNoOrders_SufficientWithZeroPendingDemand) {
    std::vector<Sample> samples = {makeSample("S009", 100)};

    auto statuses = Monitoring::stockStatus(samples, {});

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 0);
    EXPECT_EQ(statuses[0].level, StockLevel::Sufficient);
}

TEST(MonitoringStockStatusTest, MultipleWaitingOrders_SummedCorrectly) {
    std::vector<Sample> samples = {makeSample("S010", 100)};
    std::vector<Order> orders = {makeOrder("S010", OrderStatus::RESERVED, 20),
                                  makeOrder("S010", OrderStatus::RESERVED, 30),
                                  makeOrder("S010", OrderStatus::PRODUCING, 60)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 1u);
    EXPECT_EQ(statuses[0].pendingDemand, 110);
    EXPECT_EQ(statuses[0].level, StockLevel::Low);
}

TEST(MonitoringStockStatusTest, NoSamples_ReturnsEmptyList) {
    auto statuses = Monitoring::stockStatus({}, {});

    EXPECT_TRUE(statuses.empty());
}

TEST(MonitoringStockStatusTest, MultipleSamples_IndependentJudgment) {
    std::vector<Sample> samples = {makeSample("S011", 0), makeSample("S012", 10),
                                    makeSample("S013", 1000)};
    std::vector<Order> orders = {makeOrder("S012", OrderStatus::RESERVED, 50)};

    auto statuses = Monitoring::stockStatus(samples, orders);

    ASSERT_EQ(statuses.size(), 3u);
    auto findLevel = [&](const std::string& id) {
        for (const auto& s : statuses) {
            if (s.sampleId == id) return s.level;
        }
        ADD_FAILURE() << "missing " << id;
        return StockLevel::Sufficient;
    };
    EXPECT_EQ(findLevel("S011"), StockLevel::Depleted);
    EXPECT_EQ(findLevel("S012"), StockLevel::Low);
    EXPECT_EQ(findLevel("S013"), StockLevel::Sufficient);
}
