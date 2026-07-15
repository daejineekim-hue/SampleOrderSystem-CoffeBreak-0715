#include "controller/DummyDataController.h"

#include <iostream>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using view::skipToNextLine;

DummyDataController::DummyDataController(generator::DummyDataGenerator& generator)
    : generator_(generator) {}

void DummyDataController::run() {
    std::cout << "\n-- [숨김] Dummy 데이터 생성 --\n생성할 시료 더미 데이터 개수 > ";
    int count = 0;
    if (!(std::cin >> count)) {
        std::cin.clear();
        count = 0;
    }
    skipToNextLine();

    auto created = generator_.generate(count);
    std::cout << "총 " << created.size() << "건 생성 완료\n";
    for (const auto& sample : created) {
        std::cout << "  " << sample.id << " | " << sample.name
                   << " | 평균생산시간=" << sample.avgProcessTimeMin << "분 | 수율=" << sample.yieldRate
                   << " | 재고=" << sample.stock << "\n";
    }
}

}  // namespace sos::controller
