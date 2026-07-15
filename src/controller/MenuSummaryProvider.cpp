#include "controller/MenuSummaryProvider.h"

#include "model/Monitoring.h"

namespace sos::controller {

using model::Monitoring;

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

    // docs/adr/0002: reuse monitoring.md's pendingDemand definition
    // (RESERVED + PRODUCING) instead of re-deriving it here.
    auto counts = Monitoring::countByStatus(orders);
    summary.productionWaitingCount = counts.reserved + counts.producing;

    return summary;
}

}  // namespace sos::controller
