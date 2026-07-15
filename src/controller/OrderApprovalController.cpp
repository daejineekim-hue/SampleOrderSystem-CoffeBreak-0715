#include "controller/OrderApprovalController.h"

#include <iostream>
#include <stdexcept>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using model::Order;
using model::OrderStatus;
using view::skipToNextLine;

OrderApprovalController::OrderApprovalController(repository::OrderRepository& orderRepository,
                                                   production::ProductionLine& productionLine,
                                                   service::OrderLifecycleService& orderLifecycleService)
    : orderRepository_(orderRepository),
      productionLine_(productionLine),
      orderLifecycleService_(orderLifecycleService) {}

void OrderApprovalController::showPrompt() {
    std::cout << "\n-- 주문 승인/거절 --\n [1] 접수된 주문 조회  [2] 승인/거절 처리  [0] 뒤로\n선택 > ";
}

bool OrderApprovalController::handle(int choice) {
    switch (choice) {
        case 1:
            listReserved();
            return true;
        case 2:
            approveOrReject();
            return true;
        default:
            return false;
    }
}

void OrderApprovalController::listReserved() {
    bool any = false;
    for (const auto& order : orderRepository_.findAll()) {
        if (order.status != OrderStatus::RESERVED) continue;
        any = true;
        std::cout << "  " << order.orderId << " | 시료=" << order.sampleId
                   << " | 고객=" << order.customerName << " | 수량=" << order.quantity << "\n";
    }
    if (!any) std::cout << "접수된(RESERVED) 주문이 없습니다.\n";
}

void OrderApprovalController::approveOrReject() {
    std::cout << "주문번호: ";
    std::string orderId;
    std::getline(std::cin, orderId);

    std::cout << "[1] 승인  [2] 거절 > ";
    int choice = -1;
    if (!(std::cin >> choice)) {
        std::cin.clear();
        choice = -1;
    }
    skipToNextLine();

    try {
        if (choice == 1) {
            const Order& approved = orderLifecycleService_.approve(orderId, productionLine_);
            std::cout << "처리되었습니다. 상태: " << Order::statusToString(approved.status) << "\n";
        } else if (choice == 2) {
            orderLifecycleService_.reject(orderId);
            std::cout << "거절되었습니다.\n";
        } else {
            std::cout << "[오류] 유효하지 않은 선택입니다.\n";
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "[오류] " << e.what() << "\n";
    }
}

}  // namespace sos::controller
