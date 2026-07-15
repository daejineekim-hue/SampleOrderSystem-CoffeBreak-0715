#pragma once

#include <string>

#include "model/Order.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

namespace sos::service {

// User-triggered Order lifecycle transitions: approve/reject
// (docs/FEATURES/order-approval.md) and release (docs/FEATURES/shipment.md).
// Orchestrates OrderRepository (status persistence) + SampleRepository
// (stock) + ProductionLine (enqueue on insufficient stock); the legal-
// transition checks that used to live on OrderRepository live here instead.
// Production-completion (PRODUCING -> CONFIRMED once producedTotal is
// ready) is NOT here — that stays inside ProductionLine, since only it
// knows when a queued order's production time has elapsed.
class OrderLifecycleService {
public:
    OrderLifecycleService(repository::OrderRepository& orderRepository,
                           repository::SampleRepository& sampleRepository);

    // Throws std::invalid_argument if orderId doesn't exist or the order is
    // not currently RESERVED. Routes to an immediate CONFIRMED (stock
    // deducted) when sample.stock >= quantity, otherwise transitions to
    // PRODUCING and registers with productionLine.
    const model::Order& approve(const std::string& orderId, production::ProductionLine& productionLine);

    // Throws std::invalid_argument if orderId doesn't exist or the order is
    // not currently RESERVED.
    const model::Order& reject(const std::string& orderId);

    // Throws std::invalid_argument if orderId doesn't exist or the order is
    // not currently CONFIRMED. Does not touch sample stock (already
    // deducted at approval/production-completion time).
    const model::Order& release(const std::string& orderId);

private:
    repository::OrderRepository& orderRepository_;
    repository::SampleRepository& sampleRepository_;

    int stockOf(const std::string& sampleId) const;
    const model::Order& requireOrder(const std::string& orderId) const;
};

}  // namespace sos::service
