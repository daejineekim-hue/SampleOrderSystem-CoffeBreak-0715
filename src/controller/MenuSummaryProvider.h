#pragma once

#include "controller/MainMenuController.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

namespace sos::controller {

// Concrete SummaryProvider backed by the real repositories
// (docs/FEATURES/main-menu.md "Summary info displayed on entry"). Read-only:
// only queries findAll(), never mutates. productionWaitingCount follows
// monitoring.md's pendingDemand definition (RESERVED + PRODUCING orders are
// the ones still awaiting stock/shipment).
class MenuSummaryProvider : public SummaryProvider {
public:
    MenuSummaryProvider(repository::SampleRepository& sampleRepository,
                         repository::OrderRepository& orderRepository);

    MenuSummary getSummary() override;

private:
    repository::SampleRepository& sampleRepository_;
    repository::OrderRepository& orderRepository_;
};

}  // namespace sos::controller
