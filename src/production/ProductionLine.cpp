#include "production/ProductionLine.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

#include "model/Order.h"
#include "model/Sample.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

namespace sos::production {

using model::Order;
using model::Sample;
using repository::OrderRepository;
using repository::SampleRepository;

namespace {

// advanceIfDue()'s "elapsed >= productionTimeMin" boundary check and the
// producedTotal rounding both need slack against floating-point error (see
// production-line.md's "올림 처리의 부동소수점 경계" edge case).
constexpr double kEpsilon = 1e-6;

int ceilProduced(double shortage, double effectiveYield) {
    double raw = shortage / effectiveYield;
    double rounded = std::round(raw);
    if (std::abs(raw - rounded) < kEpsilon) return static_cast<int>(rounded);
    return static_cast<int>(std::ceil(raw));
}

double toMinutes(std::chrono::system_clock::duration d) {
    return std::chrono::duration<double, std::ratio<60>>(d).count();
}

}  // namespace

ProductionLine::ProductionLine(OrderRepository& orderRepository, SampleRepository& sampleRepository,
                                Clock clock)
    : orderRepository_(orderRepository), sampleRepository_(sampleRepository), clock_(std::move(clock)) {}

void ProductionLine::enqueue(const std::string& orderId) {
    queue_.push_back(QueueEntry{orderId});
    if (queue_.size() == 1) {
        startEntry(queue_.front());
    }
}

void ProductionLine::startEntry(QueueEntry& entry) {
    const Order* order = orderRepository_.findById(entry.orderId);
    if (!order) throw std::invalid_argument("생산 큐에 존재하지 않는 주문입니다: " + entry.orderId);

    int stock = 0;
    double yieldRate = 0.0;
    double avgProcessTimeMin = 0.0;
    for (const auto& sample : sampleRepository_.findAll()) {
        if (sample.id == order->sampleId) {
            stock = sample.stock;
            yieldRate = sample.yieldRate;
            avgProcessTimeMin = sample.avgProcessTimeMin;
            break;
        }
    }

    int shortage = order->quantity - stock;
    if (shortage <= 0) {
        throw std::invalid_argument("생산 라인 진입 시점의 shortage는 0보다 커야 합니다: " + entry.orderId);
    }

    double effectiveYield = yieldRate * 0.9;
    entry.producedTotal = ceilProduced(shortage, effectiveYield);
    entry.productionTimeMin = avgProcessTimeMin * entry.producedTotal;
    entry.productionStartedAt = clock_();
    entry.started = true;
}

void ProductionLine::completeHead(const QueueEntry& head) {
    const Order* order = orderRepository_.findById(head.orderId);
    if (!order) throw std::invalid_argument("생산 큐에 존재하지 않는 주문입니다: " + head.orderId);

    int currentStock = 0;
    for (const auto& sample : sampleRepository_.findAll()) {
        if (sample.id == order->sampleId) {
            currentStock = sample.stock;
            break;
        }
    }

    // Formula #3 (docs/FEATURES/production-line.md): net stock change is
    // +producedTotal, -quantity, applied together with the status flip.
    sampleRepository_.updateStock(order->sampleId, currentStock + head.producedTotal - order->quantity);
    orderRepository_.updateStatus(head.orderId, model::OrderStatus::CONFIRMED);
}

void ProductionLine::advanceIfDue() {
    auto now = clock_();
    while (!queue_.empty()) {
        QueueEntry& head = queue_.front();
        double elapsed = toMinutes(now - head.productionStartedAt);
        if (elapsed + kEpsilon < head.productionTimeMin) break;

        completeHead(head);
        queue_.pop_front();
        if (!queue_.empty()) {
            startEntry(queue_.front());
        }
    }
}

ProductionLine::Status ProductionLine::currentStatus() {
    advanceIfDue();
    Status status;
    if (queue_.empty()) return status;

    const QueueEntry& head = queue_.front();
    const Order* order = orderRepository_.findById(head.orderId);

    status.active = true;
    status.orderId = head.orderId;
    status.sampleId = order ? order->sampleId : "";
    status.quantity = order ? order->quantity : 0;
    status.producedTotal = head.producedTotal;

    double elapsed = toMinutes(clock_() - head.productionStartedAt);
    status.progressPercent = head.productionTimeMin > 0.0
                                  ? std::min(100.0, (elapsed / head.productionTimeMin) * 100.0)
                                  : 100.0;
    status.remainingMin = std::max(0.0, head.productionTimeMin - elapsed);
    return status;
}

std::vector<std::string> ProductionLine::waitingOrderIds() {
    advanceIfDue();
    std::vector<std::string> waiting;
    for (size_t i = 1; i < queue_.size(); ++i) {
        waiting.push_back(queue_[i].orderId);
    }
    return waiting;
}

}  // namespace sos::production
