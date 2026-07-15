#pragma once

#include "controller/ConsoleSubmenuController.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "service/OrderLifecycleService.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/order-approval.md (list RESERVED
// orders, approve/reject by id).
class OrderApprovalController : public ConsoleSubmenuController {
public:
    OrderApprovalController(repository::OrderRepository& orderRepository,
                             production::ProductionLine& productionLine,
                             service::OrderLifecycleService& orderLifecycleService);

protected:
    void showPrompt() override;
    bool handle(int choice) override;

private:
    repository::OrderRepository& orderRepository_;
    production::ProductionLine& productionLine_;
    service::OrderLifecycleService& orderLifecycleService_;

    void listReserved();
    void approveOrReject();
};

}  // namespace sos::controller
