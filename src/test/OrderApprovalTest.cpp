#include <gtest/gtest.h>

#include <cstdio>

#include "model/Order.h"
#include "model/Sample.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

// docs/FEATURES/order-approval.md Acceptance Criteria.

using sos::model::Order;
using sos::model::OrderStatus;
using sos::model::Sample;
using sos::production::ProductionLine;
using sos::repository::OrderRepository;
using sos::repository::SampleRepository;

namespace {

constexpr const char* kSampleFile = "order_approval_test_samples.json";
constexpr const char* kOrderFile = "order_approval_test_orders.json";

class OrderApprovalTest : public ::testing::Test {
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
        sample.stock = 15;
        sampleRepo_->create(sample);

        orderRepo_ = std::make_unique<OrderRepository>(kOrderFile, *sampleRepo_);
        productionLine_ = std::make_unique<ProductionLine>(*orderRepo_, *sampleRepo_);
    }

    void TearDown() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
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

TEST_F(OrderApprovalTest, SufficientStock_ConfirmsAndDeductsStock) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);

    const Order& approved = orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_EQ(approved.status, OrderStatus::CONFIRMED);
    EXPECT_EQ(stockOf("SMP-001"), 5);
}

TEST_F(OrderApprovalTest, StockExactlyEqualsQuantity_ConfirmsNotProducing) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 15);

    const Order& approved = orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_EQ(approved.status, OrderStatus::CONFIRMED);
    EXPECT_EQ(stockOf("SMP-001"), 0);
}

TEST_F(OrderApprovalTest, SufficientStock_NoProductionQueueEntry) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_FALSE(productionLine_->currentStatus().active);
    EXPECT_TRUE(productionLine_->waitingOrderIds().empty());
}

TEST_F(OrderApprovalTest, InsufficientStock_RoutesToProductionWithShortage) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 20);  // stock=15, shortage=5

    const Order& approved = orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_EQ(approved.status, OrderStatus::PRODUCING);
    ProductionLine::Status status = productionLine_->currentStatus();
    EXPECT_TRUE(status.active);
    EXPECT_EQ(status.orderId, order.orderId);
}

TEST_F(OrderApprovalTest, ZeroStock_RoutesToProduction) {
    orderRepo_->approve(orderRepo_->registerOrder("SMP-001", "A", 15).orderId, *productionLine_);
    // stock is now 0 after the sufficient-stock approval above.
    const Order& order = orderRepo_->registerOrder("SMP-001", "B", 5);

    const Order& approved = orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_EQ(approved.status, OrderStatus::PRODUCING);
    EXPECT_TRUE(productionLine_->currentStatus().active);
}

TEST_F(OrderApprovalTest, InsufficientStock_StockUnchangedAtApprovalTime) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 20);
    orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_EQ(stockOf("SMP-001"), 15);
}

TEST_F(OrderApprovalTest, Reject_TransitionsToRejectedImmediately) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);

    const Order& rejected = orderRepo_->reject(order.orderId);

    EXPECT_EQ(rejected.status, OrderStatus::REJECTED);
}

TEST_F(OrderApprovalTest, Reject_StockUnchanged) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->reject(order.orderId);

    EXPECT_EQ(stockOf("SMP-001"), 15);
}

TEST_F(OrderApprovalTest, Reject_NoProductionQueueEntry) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->reject(order.orderId);

    EXPECT_FALSE(productionLine_->currentStatus().active);
}

TEST_F(OrderApprovalTest, Approve_AlreadyConfirmed_Throws) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_THROW(orderRepo_->approve(order.orderId, *productionLine_), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::CONFIRMED);
}

TEST_F(OrderApprovalTest, Approve_AlreadyRejected_Throws) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->reject(order.orderId);

    EXPECT_THROW(orderRepo_->approve(order.orderId, *productionLine_), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::REJECTED);
}

TEST_F(OrderApprovalTest, Reject_AlreadyProducing_Throws) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 20);
    orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_THROW(orderRepo_->reject(order.orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::PRODUCING);
}

TEST_F(OrderApprovalTest, Reject_AlreadyConfirmed_Throws) {
    const Order& order = orderRepo_->registerOrder("SMP-001", "홍길동", 10);
    orderRepo_->approve(order.orderId, *productionLine_);

    EXPECT_THROW(orderRepo_->reject(order.orderId), std::invalid_argument);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::CONFIRMED);
}

TEST_F(OrderApprovalTest, NonexistentOrderId_ApproveThrows) {
    EXPECT_THROW(orderRepo_->approve("ORD-NOPE", *productionLine_), std::invalid_argument);
}

TEST_F(OrderApprovalTest, NonexistentOrderId_RejectThrows) {
    EXPECT_THROW(orderRepo_->reject("ORD-NOPE"), std::invalid_argument);
}

TEST_F(OrderApprovalTest, FindById_ReturnsNullptrWhenMissing) {
    EXPECT_EQ(orderRepo_->findById("ORD-NOPE"), nullptr);
}
