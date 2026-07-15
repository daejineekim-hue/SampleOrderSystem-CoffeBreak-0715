#include "controller/ShipmentController.h"

#include <iostream>
#include <stdexcept>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using view::skipToNextLine;

ShipmentController::ShipmentController(repository::OrderRepository& orderRepository)
    : orderRepository_(orderRepository) {}

void ShipmentController::run() {
    while (true) {
        std::cout << "\n-- 출고 처리 --\n [1] 출고 가능 목록  [2] 출고 실행  [0] 뒤로\n선택 > ";
        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            choice = -1;
        }
        skipToNextLine();

        if (choice == 0) return;
        switch (choice) {
            case 1:
                listShippable();
                break;
            case 2:
                releaseOrder();
                break;
            default:
                std::cout << "[오류] 유효하지 않은 선택입니다.\n";
        }
    }
}

void ShipmentController::listShippable() {
    auto shippable = orderRepository_.findShippable();
    if (shippable.empty()) {
        std::cout << "출고 가능한 주문이 없습니다.\n";
        return;
    }
    for (const auto& order : shippable) {
        std::cout << "  " << order.orderId << " | 시료=" << order.sampleId
                   << " | 고객=" << order.customerName << " | 수량=" << order.quantity << "\n";
    }
}

void ShipmentController::releaseOrder() {
    std::cout << "주문번호: ";
    std::string orderId;
    std::getline(std::cin, orderId);

    try {
        orderRepository_.release(orderId);
        std::cout << "출고 처리되었습니다.\n";
    } catch (const std::invalid_argument& e) {
        std::cout << "[오류] " << e.what() << "\n";
    }
}

}  // namespace sos::controller
