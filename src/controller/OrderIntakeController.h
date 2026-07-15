#pragma once

#include "controller/MainMenuController.h"
#include "repository/OrderRepository.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/order-intake.md (register/list).
class OrderIntakeController : public SubController {
public:
    explicit OrderIntakeController(repository::OrderRepository& orderRepository);

    void run() override;

private:
    repository::OrderRepository& orderRepository_;

    void registerOrder();
    void listOrders();
};

}  // namespace sos::controller
