#pragma once

#include "controller/MainMenuController.h"

namespace sos::view {

// Real stdin/stdout adapters for MainMenuController's InputSource/OutputSink
// seams (docs/FEATURES/main-menu.md "Testability note": actual console
// rendering is out of unit-test scope, verified manually instead).
class ConsoleInputSource : public controller::InputSource {
public:
    std::string readLine() override;
};

class ConsoleOutputSink : public controller::OutputSink {
public:
    void showMenu(const controller::MenuSummary& summary) override;
    void showError(const std::string& message) override;
    void showGoodbye() override;
};

}  // namespace sos::view
