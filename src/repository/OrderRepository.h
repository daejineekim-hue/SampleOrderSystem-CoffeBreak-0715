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

private:
    std::string filePath_;
    SampleRepository& sampleRepository_;
    Clock clock_;
    std::vector<model::Order> orders_;

    void load();
    void save() const;
    std::string nextOrderId(const std::string& datePart) const;
    bool sampleExists(const std::string& sampleId) const;
};

}  // namespace sos::repository
