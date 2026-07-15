#pragma once

#include "controller/ConsoleSubmenuController.h"
#include "repository/OrderRepository.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/order-intake.md (register/list).
class OrderIntakeController : public ConsoleSubmenuController {
public:
    explicit OrderIntakeController(repository::OrderRepository& orderRepository);

protected:
    void showPrompt() override;
    bool handle(int choice) override;

private:
    repository::OrderRepository& orderRepository_;

    void registerOrder();
    void listOrders();
};

}  // namespace sos::controller
