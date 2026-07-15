#pragma once

#include "controller/MainMenuController.h"

namespace sos::view {

// Discards the rest of the current line on std::cin, so a preceding
// `std::cin >> x` doesn't leave a trailing '\n' for the next
// std::getline() call to pick up as an empty line. Shared by every
// feature controller that mixes `>>` and getline() prompts.
void skipToNextLine();

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
