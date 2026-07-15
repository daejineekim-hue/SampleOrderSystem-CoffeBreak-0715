#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "model/Order.h"
#include "repository/SampleRepository.h"

namespace sos::production {
class ProductionLine;
}

namespace sos::repository {

// JSON-file-backed store for Order intake (docs/FEATURES/order-intake.md).
// Order IDs (ORD-YYYYMMDD-####) are derived on the fly from the existing
// orders for the same date, so no separate counter needs to be persisted and
// a failed registration never consumes a sequence number.
class OrderRepository {
public:
    using Clock = std::function<std::chrono::system_clock::time_point()>;

    OrderRepository(std::string filePath, SampleRepository& sampleRepository,
                     Clock clock = &std::chrono::system_clock::now);

    // Throws std::invalid_argument if sampleId doesn't reference a registered
    // Sample, quantity <= 0, or customerName is empty/blank.
    const model::Order& registerOrder(const std::string& sampleId,
                                       const std::string& customerName, int quantity);

    std::vector<model::Order> findAll() const;

    // Returns nullptr if no order with this id exists. Exposed so
    // ProductionLine can read order details (sampleId/quantity) without
    // duplicating storage (docs/FEATURES/production-line.md).
    const model::Order* findById(const std::string& orderId) const;

    // Approves a RESERVED order (docs/FEATURES/order-approval.md). Routes to
    // an immediate CONFIRMED (stock deducted) when sample.stock >= quantity,
    // otherwise transitions to PRODUCING and registers with productionLine.
    // Throws std::invalid_argument if orderId doesn't exist or the order is
    // not currently RESERVED.
    const model::Order& approve(const std::string& orderId, production::ProductionLine& productionLine);

    // Rejects a RESERVED order immediately (no stock change). Throws
    // std::invalid_argument if orderId doesn't exist or the order is not
    // currently RESERVED.
    const model::Order& reject(const std::string& orderId);

    // Called by ProductionLine when a queued order finishes production:
    // transitions PRODUCING -> CONFIRMED and applies the net stock change
    // (+producedTotal, -quantity). Not part of the order-intake/approval
    // public API surface.
    const model::Order& completeProduction(const std::string& orderId, int producedTotal);

private:
    std::string filePath_;
    SampleRepository& sampleRepository_;
    Clock clock_;
    std::vector<model::Order> orders_;

    void load();
    void save() const;
    std::string nextOrderId(const std::string& datePart) const;
    bool sampleExists(const std::string& sampleId) const;
    model::Order* findMutable(const std::string& orderId);
};

}  // namespace sos::repository
