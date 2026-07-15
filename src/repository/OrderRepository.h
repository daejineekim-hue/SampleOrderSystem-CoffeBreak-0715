#pragma once

#include <chrono>
#include <functional>
#include <string>
#include <vector>

#include "model/Order.h"
#include "repository/SampleRepository.h"

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

    // Orders currently eligible for shipment (status == CONFIRMED).
    std::vector<model::Order> findShippable() const;

    // Sets an order's status and persists, with no validation of whether the
    // transition is legal (same "just do it and save" contract as
    // SampleRepository::updateStock). Legal-transition checks live in
    // service::OrderLifecycleService/production::ProductionLine, which are
    // the only callers. Throws std::invalid_argument if orderId is unknown.
    const model::Order& updateStatus(const std::string& orderId, model::OrderStatus newStatus);

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
