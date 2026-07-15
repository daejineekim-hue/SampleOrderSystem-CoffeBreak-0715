#include "controller/MainMenuController.h"

#include <cctype>

namespace sos::controller {

namespace {

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

// Returns false if text isn't a valid integer (empty, whitespace, or
// containing any non-digit character other than a leading sign).
bool tryParseInt(const std::string& text, int& out) {
    std::string trimmed = trim(text);
    if (trimmed.empty()) return false;

    size_t i = 0;
    if (trimmed[0] == '-' || trimmed[0] == '+') i = 1;
    if (i >= trimmed.size()) return false;
    for (size_t j = i; j < trimmed.size(); ++j) {
        if (!std::isdigit(static_cast<unsigned char>(trimmed[j]))) return false;
    }

    out = std::stoi(trimmed);
    return true;
}

}  // namespace

MainMenuController::MainMenuController(InputSource& input, OutputSink& output,
                                        SummaryProvider& summaryProvider,
                                        SubController& sampleManagement, SubController& orderIntake,
                                        SubController& orderApproval, SubController& monitoring,
                                        SubController& productionLine, SubController& shipment)
    : input_(input),
      output_(output),
      summaryProvider_(summaryProvider),
      sampleManagement_(sampleManagement),
      orderIntake_(orderIntake),
      orderApproval_(orderApproval),
      monitoring_(monitoring),
      productionLine_(productionLine),
      shipment_(shipment) {}

void MainMenuController::run() {
    while (true) {
        output_.showMenu(summaryProvider_.getSummary());
        std::string line = input_.readLine();

        int choice = 0;
        if (!tryParseInt(line, choice)) {
            output_.showError("유효하지 않은 입력입니다. 숫자를 입력해주세요.");
            continue;
        }

        switch (choice) {
            case 0:
                output_.showGoodbye();
                return;
            case 1:
                sampleManagement_.run();
                break;
            case 2:
                orderIntake_.run();
                break;
            case 3:
                orderApproval_.run();
                break;
            case 4:
                monitoring_.run();
                break;
            case 5:
                productionLine_.run();
                break;
            case 6:
                shipment_.run();
                break;
            default:
                output_.showError("유효하지 않은 메뉴 번호입니다.");
        }
    }
}

}  // namespace sos::controller
