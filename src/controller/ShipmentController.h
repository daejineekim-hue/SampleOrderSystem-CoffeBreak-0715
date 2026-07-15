#pragma once

#include "controller/ConsoleSubmenuController.h"
#include "repository/OrderRepository.h"
#include "service/OrderLifecycleService.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/shipment.md (list CONFIRMED
// orders, release by id).
class ShipmentController : public ConsoleSubmenuController {
public:
    ShipmentController(repository::OrderRepository& orderRepository,
                        service::OrderLifecycleService& orderLifecycleService);

protected:
    void showPrompt() override;
    bool handle(int choice) override;

private:
    repository::OrderRepository& orderRepository_;
    service::OrderLifecycleService& orderLifecycleService_;

    void listShippable();
    void releaseOrder();
};

}  // namespace sos::controller
