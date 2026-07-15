#include "controller/ShipmentController.h"

#include <iostream>
#include <stdexcept>

namespace sos::controller {

ShipmentController::ShipmentController(repository::OrderRepository& orderRepository,
                                        service::OrderLifecycleService& orderLifecycleService)
    : orderRepository_(orderRepository), orderLifecycleService_(orderLifecycleService) {}

void ShipmentController::showPrompt() {
    std::cout << "\n-- 출고 처리 --\n [1] 출고 가능 목록  [2] 출고 실행  [0] 뒤로\n선택 > ";
}

bool ShipmentController::handle(int choice) {
    switch (choice) {
        case 1:
            listShippable();
            return true;
        case 2:
            releaseOrder();
            return true;
        default:
            return false;
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
        orderLifecycleService_.release(orderId);
        std::cout << "출고 처리되었습니다.\n";
    } catch (const std::invalid_argument& e) {
        std::cout << "[오류] " << e.what() << "\n";
    }
}

}  // namespace sos::controller
