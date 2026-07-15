#pragma once

#include <string>
#include <vector>

#include "model/Order.h"
#include "model/Sample.h"

namespace sos::model {

// Read-only aggregation over already-loaded Order/Sample lists
// (docs/FEATURES/monitoring.md). Pure computation, no I/O: callers fetch the
// lists from the repositories and pass them in.
struct OrderCountByStatus {
    int reserved = 0;
    int confirmed = 0;
    int producing = 0;
    int release = 0;
};

enum class StockLevel { Sufficient, Low, Depleted };

struct StockStatus {
    std::string sampleId;
    int stock = 0;
    int pendingDemand = 0;
    StockLevel level = StockLevel::Sufficient;
};

class Monitoring {
public:
    // REJECTED orders are excluded entirely.
    static OrderCountByStatus countByStatus(const std::vector<Order>& orders);

    // pendingDemand per sample = sum of quantity for that sample's RESERVED/
    // PRODUCING orders (CONFIRMED/RELEASE/REJECTED are excluded). Judgment
    // order: stock==0 -> Depleted; else stock>=pendingDemand -> Sufficient;
    // else Low.
    static std::vector<StockStatus> stockStatus(const std::vector<Sample>& samples,
                                                 const std::vector<Order>& orders);
};

}  // namespace sos::model
