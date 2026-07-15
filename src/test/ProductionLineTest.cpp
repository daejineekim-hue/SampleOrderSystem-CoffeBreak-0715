#include <gtest/gtest.h>

#include <cstdio>
#include <ctime>

#include "model/Order.h"
#include "model/Sample.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"
#include "service/OrderLifecycleService.h"

// docs/FEATURES/production-line.md Acceptance Criteria.

using sos::model::Order;
using sos::model::OrderStatus;
using sos::model::Sample;
using sos::production::ProductionLine;
using sos::repository::OrderRepository;
using sos::repository::SampleRepository;
using sos::service::OrderLifecycleService;

namespace {

constexpr const char* kSampleFile = "production_line_test_samples.json";
constexpr const char* kOrderFile = "production_line_test_orders.json";

// advanceIfDue()'s elapsed>=productionTimeMin check tolerates this much
// floating-point slack; kept in sync with the production code's epsilon so
// the "exact division / exact boundary" tests are meaningful.
constexpr double kEpsilonMin = 1e-6;

class ProductionLineTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
        sampleRepo_ = std::make_unique<SampleRepository>(kSampleFile);
        orderRepo_ = std::make_unique<OrderRepository>(kOrderFile, *sampleRepo_);
        now_ = FixedStart();
        clock_ = [this]() { return now_; };
        line_ = std::make_unique<ProductionLine>(*orderRepo_, *sampleRepo_, clock_);
        service_ = std::make_unique<OrderLifecycleService>(*orderRepo_, *sampleRepo_);
    }

    void TearDown() override {
        std::remove(kSampleFile);
        std::remove(kOrderFile);
    }

    static std::chrono::system_clock::time_point FixedStart() {
        std::tm tmv{};
        tmv.tm_year = 2026 - 1900;
        tmv.tm_mon = 3;
        tmv.tm_mday = 16;
        tmv.tm_hour = 9;
        tmv.tm_min = 0;
        tmv.tm_sec = 0;
        tmv.tm_isdst = -1;
        return std::chrono::system_clock::from_time_t(std::mktime(&tmv));
    }

    void advanceMinutes(double minutes) {
        now_ += std::chrono::duration_cast<std::chrono::system_clock::duration>(
            std::chrono::duration<double, std::ratio<60>>(minutes));
    }

    void addSample(const std::string& id, double avgProcessTimeMin, double yieldRate, int stock) {
        Sample s;
        s.id = id;
        s.name = id;
        s.avgProcessTimeMin = avgProcessTimeMin;
        s.yieldRate = yieldRate;
        s.stock = stock;
        sampleRepo_->create(s);
    }

    int stockOf(const std::string& id) {
        for (const auto& s : sampleRepo_->findAll()) {
            if (s.id == id) return s.stock;
        }
        ADD_FAILURE() << "sample not found: " << id;
        return -1;
    }

    // Registers + approves; asserts the order was routed to PRODUCING.
    // Returns a copy (not a reference) because registering later orders can
    // reallocate OrderRepository's internal vector, which would dangle any
    // reference into it taken earlier.
    Order approveIntoProduction(const std::string& sampleId, const std::string& customer,
                                 int quantity) {
        std::string orderId = orderRepo_->registerOrder(sampleId, customer, quantity).orderId;
        const Order& approved = service_->approve(orderId, *line_);
        EXPECT_EQ(approved.status, OrderStatus::PRODUCING);
        return approved;
    }

    std::unique_ptr<SampleRepository> sampleRepo_;
    std::unique_ptr<OrderRepository> orderRepo_;
    std::chrono::system_clock::time_point now_;
    ProductionLine::Clock clock_;
    std::unique_ptr<ProductionLine> line_;
    std::unique_ptr<OrderLifecycleService> service_;
};

}  // namespace

// ---- Formulas ----

TEST_F(ProductionLineTest, WorkedExample_Shortage50_Yield092) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    const Order& order = approveIntoProduction("SMP-001", "A", 80);

    ProductionLine::Status status = line_->currentStatus();

    EXPECT_TRUE(status.active);
    EXPECT_EQ(status.orderId, order.orderId);
    EXPECT_EQ(status.producedTotal, 61);
    EXPECT_NEAR(status.remainingMin, 1220.0, kEpsilonMin);
}

TEST_F(ProductionLineTest, ExactDivision_DoesNotOverRound) {
    addSample("SMP-002", /*avgProcessTimeMin=*/1, /*yieldRate=*/0.9, /*stock=*/0);
    const Order& order = approveIntoProduction("SMP-002", "A", 81);  // shortage=81

    ProductionLine::Status status = line_->currentStatus();

    EXPECT_EQ(status.orderId, order.orderId);
    EXPECT_EQ(status.producedTotal, 100);
}

// ---- advanceIfDue() ----

TEST_F(ProductionLineTest, NotYetDue_StillProducingWithProgress) {
    // yieldRate=1.0 (max), shortage=1 -> producedTotal=ceil(1/0.9)=2, avgProcessTimeMin=50 -> 100min.
    addSample("SMP-001", /*avgProcessTimeMin=*/50, /*yieldRate=*/1.0, /*stock=*/0);
    approveIntoProduction("SMP-001", "A", 1);

    advanceMinutes(40);
    ProductionLine::Status status = line_->currentStatus();

    EXPECT_TRUE(status.active);
    EXPECT_NEAR(status.progressPercent, 40.0, kEpsilonMin);
    EXPECT_NEAR(status.remainingMin, 60.0, kEpsilonMin);
}

TEST_F(ProductionLineTest, ExactlyAtBoundary_Completes) {
    addSample("SMP-001", /*avgProcessTimeMin=*/50, /*yieldRate=*/1.0, /*stock=*/0);
    const Order& order = approveIntoProduction("SMP-001", "A", 1);

    advanceMinutes(100);
    line_->advanceIfDue();

    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::CONFIRMED);
}

TEST_F(ProductionLineTest, Completes_UpdatesStockAndStatus) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    const Order& order = approveIntoProduction("SMP-001", "A", 80);  // producedTotal=61, 1220min

    advanceMinutes(1220);
    line_->advanceIfDue();

    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_EQ(stockOf("SMP-001"), 11);  // 30 + 61 - 80
}

TEST_F(ProductionLineTest, Completes_PromotesNextWaitingOrder_RecalculatesFormula) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    const Order& first = approveIntoProduction("SMP-001", "A", 80);   // producedTotal=61, 1220min
    const Order& second = approveIntoProduction("SMP-001", "B", 35);  // stock still 30 at approval time

    advanceMinutes(1220);
    ProductionLine::Status status = line_->currentStatus();

    EXPECT_EQ(orderRepo_->findById(first.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_TRUE(status.active);
    EXPECT_EQ(status.orderId, second.orderId);
    // stock after first completes = 11; shortage = 35 - 11 = 24; effective yield = 0.828
    // raw = 24 / 0.828 = 28.985... -> ceil = 29 -> productionTimeMin = 20*29 = 580
    EXPECT_EQ(status.producedTotal, 29);
    EXPECT_NEAR(status.progressPercent, 0.0, kEpsilonMin);
    EXPECT_NEAR(status.remainingMin, 580.0, kEpsilonMin);
}

TEST_F(ProductionLineTest, CascadeCompletion_MultipleOrdersInOneCall) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    // Vanishingly small production time so, once promoted at "now", it is
    // immediately due within the same advanceIfDue() call.
    addSample("SMP-002", /*avgProcessTimeMin=*/1e-9, /*yieldRate=*/1.0, /*stock=*/0);

    const Order& first = approveIntoProduction("SMP-001", "A", 80);   // 1220min
    const Order& second = approveIntoProduction("SMP-002", "B", 1);   // negligible production time

    advanceMinutes(1220);
    line_->advanceIfDue();

    EXPECT_EQ(orderRepo_->findById(first.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_EQ(orderRepo_->findById(second.orderId)->status, OrderStatus::CONFIRMED);
    EXPECT_FALSE(line_->currentStatus().active);
    EXPECT_EQ(stockOf("SMP-001"), 11);
    EXPECT_EQ(stockOf("SMP-002"), 1);  // 0 + 2 - 1
}

TEST_F(ProductionLineTest, EmptyQueue_AdvanceIfDueIsNoOp) {
    EXPECT_NO_THROW(line_->advanceIfDue());
    EXPECT_FALSE(line_->currentStatus().active);
}

// ---- Display Production Status ----

TEST_F(ProductionLineTest, ShowsCurrentProducingInfo) {
    addSample("SMP-001", /*avgProcessTimeMin=*/50, /*yieldRate=*/1.0, /*stock=*/0);
    const Order& order = approveIntoProduction("SMP-001", "A", 1);

    advanceMinutes(40);
    ProductionLine::Status status = line_->currentStatus();

    EXPECT_EQ(status.orderId, order.orderId);
    EXPECT_EQ(status.sampleId, "SMP-001");
    EXPECT_EQ(status.quantity, 1);
    EXPECT_EQ(status.producedTotal, 2);
    EXPECT_NEAR(status.progressPercent, 40.0, kEpsilonMin);
    EXPECT_NEAR(status.remainingMin, 60.0, kEpsilonMin);
}

TEST_F(ProductionLineTest, NoProducingOrder_StatusInactive) {
    ProductionLine::Status status = line_->currentStatus();

    EXPECT_FALSE(status.active);
}

TEST_F(ProductionLineTest, QueryAppliesLazyCompletion_BeforeReturning) {
    addSample("SMP-001", /*avgProcessTimeMin=*/50, /*yieldRate=*/1.0, /*stock=*/0);
    const Order& order = approveIntoProduction("SMP-001", "A", 1);

    advanceMinutes(100);
    ProductionLine::Status status = line_->currentStatus();  // no explicit advanceIfDue() call

    EXPECT_FALSE(status.active);
    EXPECT_EQ(orderRepo_->findById(order.orderId)->status, OrderStatus::CONFIRMED);
}

// ---- Waiting Queue ----

TEST_F(ProductionLineTest, WaitingOrders_ReturnedInFifoOrderExcludingHead) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    const Order& first = approveIntoProduction("SMP-001", "A", 35);
    const Order& second = approveIntoProduction("SMP-001", "B", 36);
    const Order& third = approveIntoProduction("SMP-001", "C", 37);

    std::vector<std::string> waiting = line_->waitingOrderIds();

    ASSERT_EQ(waiting.size(), 2u);
    EXPECT_EQ(waiting[0], second.orderId);
    EXPECT_EQ(waiting[1], third.orderId);
    EXPECT_NE(waiting[0], first.orderId);
}

TEST_F(ProductionLineTest, NoWaitingOrders_ReturnsEmpty) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    approveIntoProduction("SMP-001", "A", 35);

    EXPECT_TRUE(line_->waitingOrderIds().empty());
}

TEST_F(ProductionLineTest, WaitingOrders_ReflectsLatestStateAfterCompletion) {
    addSample("SMP-001", /*avgProcessTimeMin=*/20, /*yieldRate=*/0.92, /*stock=*/30);
    approveIntoProduction("SMP-001", "A", 80);          // 1220min
    const Order& second = approveIntoProduction("SMP-001", "B", 35);
    const Order& third = approveIntoProduction("SMP-001", "C", 90);

    advanceMinutes(1220);
    std::vector<std::string> waiting = line_->waitingOrderIds();

    ASSERT_EQ(waiting.size(), 1u);
    EXPECT_EQ(waiting[0], third.orderId);
    EXPECT_NE(waiting[0], second.orderId);
}
