#pragma once

#include "controller/MainMenuController.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

namespace sos::controller {

// Console-driven read-only view for docs/FEATURES/monitoring.md (order
// count by status, stock status by sample). Aggregation itself is
// Monitoring::countByStatus/stockStatus, already covered by MonitoringTest.
class MonitoringController : public SubController {
public:
    MonitoringController(repository::SampleRepository& sampleRepository,
                          repository::OrderRepository& orderRepository);

    void run() override;

private:
    repository::SampleRepository& sampleRepository_;
    repository::OrderRepository& orderRepository_;
};

}  // namespace sos::controller
