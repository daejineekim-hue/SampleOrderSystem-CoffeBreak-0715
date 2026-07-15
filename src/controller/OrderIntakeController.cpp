#include "controller/OrderIntakeController.h"

#include <iostream>
#include <stdexcept>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using model::Order;
using view::skipToNextLine;

namespace {

void printOrder(const Order& order) {
    std::cout << "  " << order.orderId << " | 시료=" << order.sampleId << " | 고객=" << order.customerName
               << " | 수량=" << order.quantity << " | 상태=" << Order::statusToString(order.status)
               << " | 접수시각=" << order.createdAt << "\n";
}

}  // namespace

OrderIntakeController::OrderIntakeController(repository::OrderRepository& orderRepository)
    : orderRepository_(orderRepository) {}

void OrderIntakeController::run() {
    while (true) {
        std::cout << "\n-- 시료 주문 --\n [1] 접수  [2] 전체 조회  [0] 뒤로\n선택 > ";
        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            choice = -1;
        }
        skipToNextLine();

        if (choice == 0) return;
        switch (choice) {
            case 1:
                registerOrder();
                break;
            case 2:
                listOrders();
                break;
            default:
                std::cout << "[오류] 유효하지 않은 선택입니다.\n";
        }
    }
}

void OrderIntakeController::registerOrder() {
    std::string sampleId;
    std::string customerName;
    int quantity = 0;

    std::cout << "시료 ID: ";
    std::getline(std::cin, sampleId);
    std::cout << "고객명: ";
    std::getline(std::cin, customerName);
    std::cout << "수량: ";
    std::cin >> quantity;
    skipToNextLine();

    try {
        const Order& order = orderRepository_.registerOrder(sampleId, customerName, quantity);
        std::cout << "접수되었습니다. 주문번호: " << order.orderId << "\n";
    } catch (const std::invalid_argument& e) {
        std::cout << "[오류] " << e.what() << "\n";
    }
}

void OrderIntakeController::listOrders() {
    auto orders = orderRepository_.findAll();
    if (orders.empty()) {
        std::cout << "접수된 주문이 없습니다.\n";
        return;
    }
    for (const auto& order : orders) printOrder(order);
}

}  // namespace sos::controller
