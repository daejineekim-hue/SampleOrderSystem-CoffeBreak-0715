#include "controller/ProductionLineController.h"

#include <iostream>

namespace sos::controller {

ProductionLineController::ProductionLineController(production::ProductionLine& productionLine)
    : productionLine_(productionLine) {}

void ProductionLineController::run() {
    auto status = productionLine_.currentStatus();

    std::cout << "\n-- 생산라인 조회 --\n";
    if (!status.active) {
        std::cout << "  현재 생산 중인 주문이 없습니다.\n";
    } else {
        std::cout << "  " << status.orderId << " | 시료=" << status.sampleId
                   << " | 수량=" << status.quantity << " | 생산예정량=" << status.producedTotal
                   << " | 진행률=" << status.progressPercent << "%"
                   << " | 남은시간=" << status.remainingMin << "분\n";
    }

    auto waiting = productionLine_.waitingOrderIds();
    std::cout << "  대기 중: ";
    if (waiting.empty()) {
        std::cout << "없음\n";
    } else {
        for (const auto& orderId : waiting) std::cout << orderId << " ";
        std::cout << "\n";
    }
}

}  // namespace sos::controller
