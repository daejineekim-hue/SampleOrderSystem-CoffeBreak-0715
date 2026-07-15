#pragma once

#include "controller/MainMenuController.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/order-approval.md (list RESERVED
// orders, approve/reject by id).
class OrderApprovalController : public SubController {
public:
    OrderApprovalController(repository::OrderRepository& orderRepository,
                             production::ProductionLine& productionLine);

    void run() override;

private:
    repository::OrderRepository& orderRepository_;
    production::ProductionLine& productionLine_;

    void listReserved();
    void approveOrReject();
};

}  // namespace sos::controller
