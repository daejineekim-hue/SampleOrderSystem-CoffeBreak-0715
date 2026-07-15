#include "controller/MonitoringController.h"

#include <iostream>
#include <limits>

#include "model/Monitoring.h"

namespace sos::controller {

using model::Monitoring;
using model::StockLevel;

namespace {

const char* levelToKorean(StockLevel level) {
    switch (level) {
        case StockLevel::Sufficient:
            return "여유";
        case StockLevel::Low:
            return "부족";
        case StockLevel::Depleted:
            return "고갈";
    }
    return "알수없음";
}

}  // namespace

MonitoringController::MonitoringController(repository::SampleRepository& sampleRepository,
                                            repository::OrderRepository& orderRepository)
    : sampleRepository_(sampleRepository), orderRepository_(orderRepository) {}

void MonitoringController::run() {
    auto orders = orderRepository_.findAll();
    auto samples = sampleRepository_.findAll();

    auto counts = Monitoring::countByStatus(orders);
    std::cout << "\n-- 주문량 확인 --\n";
    std::cout << "  RESERVED=" << counts.reserved << "  CONFIRMED=" << counts.confirmed
               << "  PRODUCING=" << counts.producing << "  RELEASE=" << counts.release << "\n";

    auto statuses = Monitoring::stockStatus(samples, orders);
    std::cout << "\n-- 재고량 확인 --\n";
    if (statuses.empty()) {
        std::cout << "  등록된 시료가 없습니다.\n";
    }
    for (const auto& status : statuses) {
        std::cout << "  " << status.sampleId << " | 재고=" << status.stock
                   << " | 대기수요=" << status.pendingDemand << " | 상태=" << levelToKorean(status.level)
                   << "\n";
    }

    std::cout << "\n계속하려면 Enter를 누르세요...";
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

}  // namespace sos::controller
