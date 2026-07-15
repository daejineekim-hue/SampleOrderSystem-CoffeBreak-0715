#pragma once

#include "controller/MainMenuController.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"

namespace sos::controller {

// Console-driven read-only view for docs/FEATURES/monitoring.md (order
// count by status, stock status by sample). Aggregation itself is
// Monitoring::countByStatus/stockStatus, already covered by MonitoringTest.
//
// Deliberately does NOT end with a "press Enter to continue" prompt: this is
// only ever invoked from MainMenuController, which reads the next menu
// number via getline() (already consumes the trailing newline). A trailing
// cin.ignore() here would have nothing left to skip and would instead
// swallow the user's next real keystroke (see system-test.ps1 scenario 7).
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
