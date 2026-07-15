#include "controller/ConsoleSubmenuController.h"

#include <iostream>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using view::skipToNextLine;

void ConsoleSubmenuController::run() {
    while (true) {
        showPrompt();
        int choice = -1;
        if (!(std::cin >> choice)) {
            std::cin.clear();
            choice = -1;
        }
        skipToNextLine();

        if (choice == 0) return;
        if (!handle(choice)) {
            std::cout << "[오류] 유효하지 않은 선택입니다.\n";
        }
    }
}

}  // namespace sos::controller
