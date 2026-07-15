#include <gtest/gtest.h>

#include <algorithm>
#include <cstdio>

#include "model/Order.h"
#include "model/Sample.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

// docs/FEATURES/shipment.md Acceptance Criteria.

using sos::model::Order;
using sos::model::OrderStatus;
using sos::model::Sample;
using sos::production::ProductionLine;
using sos::repository::OrderRepository;
using sos::repository::SampleRepository;

namespace {

constexpr const char* kSampleFile = "shipment_test_samples.json";
constexpr const char* kOrderFile = "shipment_test_orders.json";

class ShipmentTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
        sampleRepo_ = std::make_unique<SampleRepository>(kSampleFile);

        Sample sample;
        sample.id = "SMP-001";
        sample.name = "Wafer-A";
        sample.avgProcessTimeMin = 20;
        sample.yieldRate = 0.9;
        sample.stock = 50;
        sampleRepo_->create(sample);

        orderRepo_ = std::make_unique<OrderRepository>(kOrderFile, *sampleRepo_);
        productionLine_ = std::make_unique<ProductionLine>(*orderRepo_, *sampleRepo_);
    }

    void TearDown() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
    }

    // Registers, approves (sufficient stock -> CONFIRMED), returns the orderId.
    std::string confirmedOrder(const std::string& customer, int quantity) {
        std::string orderId = orderRepo_->registerOrder("SMP-001", customer, quantity).orderId;
        const Order& approved = orderRepo_->approve(orderId, *productionLine_);
        EXPECT_EQ(approved.status, OrderStatus::CONFIRMED);
        return orderId;
    }

    int stockOf(const std::string& sampleId) {
        for (const auto& s : sampleRepo_->findAll()) {
            if (s.id == sampleId) return s.stock;
        }
        ADD_FAILURE() << "sample not found: " << sampleId;
        return -1;
    }

    std::unique_ptr<SampleRepository> sampleRepo_;
    std::unique_ptr<OrderRepository> orderRepo_;
    std::unique_ptr<ProductionLine> productionLine_;
};

}  // namespace

// ---- 출고 실행 (Execute Shipment) ----

TEST_F(ShipmentTest, Confirmed_TransitionsToRelease) {
    std::string orderId = confirmedOrder("A", 10);

    const Order& released = orderRepo_->release(orderId);

    EXPECT_EQ(released.status, OrderStatus::RELEASE);
}

TEST_F(ShipmentTest, Success_ReflectedOnSubsequentLookup) {
    std::string orderId = confirmedOrder("A", 10);
    orderRepo_->release(orderId);

    EXPECT_EQ(orderRepo_->findById(orderId)->status, OrderStatus::RELEASE);
}

TEST_F(ShipmentTest, MultipleOrders_ReleaseIsIndependent) {
    std::string o1 = confirmedOrder("A", 10);
    std::string o2 = confirmedOrder("B", 10);

    orderRepo_->release(o1);

    EXPECT_EQ(orderRepo_->findById(o1)->status, OrderStatus::RELEASE);
    EXPECT_EQ(orderRepo_->findById(o2)->status, OrderStatus::CONFIRMED);
}

TEST_F(ShipmentTest, DoesNotReDeductStock) {
    std::string orderId = confirmedOrder("A", 30);
    int stockBefore = stockOf("SMP-001");

    orderRepo_->release(orderId);

    EXPECT_EQ(stockOf("SMP-001"), stockBefore);
}

TEST_F(ShipmentTest, Reserved_ThrowsAndLeavesStatusUnchanged) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "A", 10);

    EXPECT_THROW(orderRepo_->release(order.orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::RESERVED);
}

TEST_F(ShipmentTest, Producing_ThrowsAndLeavesStatusUnchanged) {
    std::string orderId = orderRepo_->registerOrder("SMP-001", "A", 100).orderId;  // stock=50 -> PRODUCING
    orderRepo_->approve(orderId, *productionLine_);

    EXPECT_THROW(orderRepo_->release(orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(orderId)->status, OrderStatus::PRODUCING);
}

TEST_F(ShipmentTest, AlreadyReleased_ThrowsAndLeavesStatusUnchanged) {
    std::string orderId = confirmedOrder("A", 10);
    orderRepo_->release(orderId);

    EXPECT_THROW(orderRepo_->release(orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(orderId)->status, OrderStatus::RELEASE);
}

TEST_F(ShipmentTest, Rejected_ThrowsAndLeavesStatusUnchanged) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "A", 10);
    orderRepo_->reject(order.orderId);

    EXPECT_THROW(orderRepo_->release(order.orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::REJECTED);
}

TEST_F(ShipmentTest, NonexistentOrderId_Throws) {
    EXPECT_THROW(orderRepo_->release("ORD-NOPE"), std::invalid_argument);
}

// ---- 출고 가능 목록 조회 (List Shippable Orders) ----

TEST_F(ShipmentTest, FindShippable_FiltersConfirmedOnly) {
    std::string confirmed1 = confirmedOrder("A", 10);
    std::string reserved = orderRepo_->registerOrder("SMP-001", "B", 5).orderId;
    std::string producing = orderRepo_->registerOrder("SMP-001", "C", 100).orderId;
    orderRepo_->approve(producing, *productionLine_);
    std::string released = confirmedOrder("D", 5);
    orderRepo_->release(released);
    std::string rejected = orderRepo_->registerOrder("SMP-001", "E", 5).orderId;
    orderRepo_->reject(rejected);
    std::string confirmed2 = confirmedOrder("F", 5);

    std::vector<Order> shippable = orderRepo_->findShippable();

    std::vector<std::string> ids;
    for (const auto& o : shippable) ids.push_back(o.orderId);
    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(std::find(ids.begin(), ids.end(), confirmed1), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), confirmed2), ids.end());
}

TEST_F(ShipmentTest, FindShippable_EmptyWhenNoneConfirmed) {
    orderRepo_->registerOrder("SMP-001", "A", 5);  // RESERVED only

    EXPECT_TRUE(orderRepo_->findShippable().empty());
}

TEST_F(ShipmentTest, FindShippable_EmptyWhenNoOrders) {
    EXPECT_TRUE(orderRepo_->findShippable().empty());
}

TEST_F(ShipmentTest, FindShippable_ExcludesReleasedAfterRelease) {
    std::string o1 = confirmedOrder("A", 10);
    std::string o2 = confirmedOrder("B", 10);
    orderRepo_->release(o1);

    std::vector<Order> shippable = orderRepo_->findShippable();

    ASSERT_EQ(shippable.size(), 1u);
    EXPECT_EQ(shippable[0].orderId, o2);
}
