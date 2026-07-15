#include "controller/MenuSummaryProvider.h"

namespace sos::controller {

using model::OrderStatus;

MenuSummaryProvider::MenuSummaryProvider(repository::SampleRepository& sampleRepository,
                                          repository::OrderRepository& orderRepository)
    : sampleRepository_(sampleRepository), orderRepository_(orderRepository) {}

MenuSummary MenuSummaryProvider::getSummary() {
    MenuSummary summary;

    auto samples = sampleRepository_.findAll();
    summary.totalSamples = static_cast<int>(samples.size());
    for (const auto& sample : samples) summary.totalStock += sample.stock;

    auto orders = orderRepository_.findAll();
    summary.totalOrders = static_cast<int>(orders.size());
    for (const auto& order : orders) {
        if (order.status == OrderStatus::RESERVED || order.status == OrderStatus::PRODUCING) {
            ++summary.productionWaitingCount;
        }
    }

    return summary;
}

}  // namespace sos::controller
