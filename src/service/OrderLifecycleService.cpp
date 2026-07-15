#include "service/OrderLifecycleService.h"

#include <stdexcept>

namespace sos::service {

using model::Order;
using model::OrderStatus;

OrderLifecycleService::OrderLifecycleService(repository::OrderRepository& orderRepository,
                                              repository::SampleRepository& sampleRepository)
    : orderRepository_(orderRepository), sampleRepository_(sampleRepository) {}

int OrderLifecycleService::stockOf(const std::string& sampleId) const {
    for (const auto& sample : sampleRepository_.findAll()) {
        if (sample.id == sampleId) return sample.stock;
    }
    return 0;
}

const Order& OrderLifecycleService::requireOrder(const std::string& orderId) const {
    const Order* order = orderRepository_.findById(orderId);
    if (!order) throw std::invalid_argument("존재하지 않는 주문 ID입니다: " + orderId);
    return *order;
}

const Order& OrderLifecycleService::approve(const std::string& orderId,
                                             production::ProductionLine& productionLine) {
    const Order& order = requireOrder(orderId);
    if (order.status != OrderStatus::RESERVED) {
        throw std::invalid_argument("RESERVED 상태의 주문만 승인할 수 있습니다: " + orderId);
    }

    int currentStock = stockOf(order.sampleId);
    if (currentStock >= order.quantity) {
        sampleRepository_.updateStock(order.sampleId, currentStock - order.quantity);
        return orderRepository_.updateStatus(orderId, OrderStatus::CONFIRMED);
    }

    const Order& producing = orderRepository_.updateStatus(orderId, OrderStatus::PRODUCING);
    productionLine.enqueue(orderId);
    return producing;
}

const Order& OrderLifecycleService::reject(const std::string& orderId) {
    const Order& order = requireOrder(orderId);
    if (order.status != OrderStatus::RESERVED) {
        throw std::invalid_argument("RESERVED 상태의 주문만 거절할 수 있습니다: " + orderId);
    }
    return orderRepository_.updateStatus(orderId, OrderStatus::REJECTED);
}

const Order& OrderLifecycleService::release(const std::string& orderId) {
    const Order& order = requireOrder(orderId);
    if (order.status != OrderStatus::CONFIRMED) {
        throw std::invalid_argument("CONFIRMED 상태의 주문만 출고할 수 있습니다: " + orderId);
    }
    return orderRepository_.updateStatus(orderId, OrderStatus::RELEASE);
}

}  // namespace sos::service
