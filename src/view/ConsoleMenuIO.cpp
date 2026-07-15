#include "view/ConsoleMenuIO.h"

#include <iostream>

namespace sos::view {

using controller::MenuSummary;

std::string ConsoleInputSource::readLine() {
    std::string line;
    if (!std::getline(std::cin, line)) return "0";  // EOF/closed stdin -> exit main loop
    return line;
}

void ConsoleOutputSink::showMenu(const MenuSummary& summary) {
    std::cout << "\n==================================\n";
    std::cout << " 반도체 시료 생산주문관리 시스템\n";
    std::cout << "----------------------------------\n";
    std::cout << " 등록 시료 수: " << summary.totalSamples << "  |  총 재고: " << summary.totalStock
               << "\n";
    std::cout << " 전체 주문 수: " << summary.totalOrders
               << "  |  생산라인 대기: " << summary.productionWaitingCount << "\n";
    std::cout << "----------------------------------\n";
    std::cout << " [1] 시료 관리\n";
    std::cout << " [2] 시료 주문\n";
    std::cout << " [3] 주문 승인/거절\n";
    std::cout << " [4] 모니터링\n";
    std::cout << " [5] 생산라인 조회\n";
    std::cout << " [6] 출고 처리\n";
    std::cout << " [0] 종료\n";
    std::cout << "==================================\n";
    std::cout << "선택 > ";
}

void ConsoleOutputSink::showError(const std::string& message) { std::cout << "[오류] " << message << "\n"; }

void ConsoleOutputSink::showGoodbye() { std::cout << "\n시스템을 종료합니다.\n"; }

}  // namespace sos::view
