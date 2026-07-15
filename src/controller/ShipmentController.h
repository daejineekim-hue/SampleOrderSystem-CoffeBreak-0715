#pragma once

#include "controller/MainMenuController.h"
#include "repository/OrderRepository.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/shipment.md (list CONFIRMED
// orders, release by id).
class ShipmentController : public SubController {
public:
    explicit ShipmentController(repository::OrderRepository& orderRepository);

    void run() override;

private:
    repository::OrderRepository& orderRepository_;

    void listShippable();
    void releaseOrder();
};

}  // namespace sos::controller
