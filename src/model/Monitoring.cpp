#include "model/Monitoring.h"

namespace sos::model {

OrderCountByStatus Monitoring::countByStatus(const std::vector<Order>& orders) {
    OrderCountByStatus counts;
    for (const auto& order : orders) {
        switch (order.status) {
            case OrderStatus::RESERVED:
                ++counts.reserved;
                break;
            case OrderStatus::CONFIRMED:
                ++counts.confirmed;
                break;
            case OrderStatus::PRODUCING:
                ++counts.producing;
                break;
            case OrderStatus::RELEASE:
                ++counts.release;
                break;
            case OrderStatus::REJECTED:
                break;
        }
    }
    return counts;
}

std::vector<StockStatus> Monitoring::stockStatus(const std::vector<Sample>& samples,
                                                  const std::vector<Order>& orders) {
    std::vector<StockStatus> result;
    for (const auto& sample : samples) {
        int pendingDemand = 0;
        for (const auto& order : orders) {
            if (order.sampleId != sample.id) continue;
            if (order.status == OrderStatus::RESERVED || order.status == OrderStatus::PRODUCING) {
                pendingDemand += order.quantity;
            }
        }

        StockStatus status;
        status.sampleId = sample.id;
        status.stock = sample.stock;
        status.pendingDemand = pendingDemand;
        if (sample.stock == 0) {
            status.level = StockLevel::Depleted;
        } else if (sample.stock >= pendingDemand) {
            status.level = StockLevel::Sufficient;
        } else {
            status.level = StockLevel::Low;
        }
        result.push_back(status);
    }
    return result;
}

}  // namespace sos::model
