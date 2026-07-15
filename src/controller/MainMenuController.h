#pragma once

#include <string>

namespace sos::controller {

// A dispatchable feature entry point (sample management, order intake, ...).
// Concrete implementations do their own console I/O; MainMenuController only
// calls run() once per selection (docs/FEATURES/main-menu.md).
class SubController {
public:
    virtual ~SubController() = default;
    virtual void run() = 0;
};

// Abstracts reading one line of raw menu input, so tests can inject a
// pre-scripted sequence instead of reading real stdin.
class InputSource {
public:
    virtual ~InputSource() = default;
    virtual std::string readLine() = 0;
};

// Read-only summary shown alongside the menu (docs/FEATURES/main-menu.md
// "Summary info displayed on entry"). MainMenuController does not compute
// these values itself; it only displays what SummaryProvider returns.
struct MenuSummary {
    int totalSamples = 0;
    int totalStock = 0;
    int totalOrders = 0;
    int productionWaitingCount = 0;
};

class SummaryProvider {
public:
    virtual ~SummaryProvider() = default;
    virtual MenuSummary getSummary() = 0;
};

// Abstracts all menu-related console output, so tests can inject a recording
// no-op instead of writing to real stdout.
class OutputSink {
public:
    virtual ~OutputSink() = default;
    virtual void showMenu(const MenuSummary& summary) = 0;
    virtual void showError(const std::string& message) = 0;
    virtual void showGoodbye() = 0;
};

// Main menu routing (docs/FEATURES/main-menu.md). Reads a menu number via
// InputSource, validates it, and dispatches to the matching SubController.
// Invalid input shows an error and re-prompts; "0" ends run().
class MainMenuController {
public:
    // dummyData is a hidden option (menu choice 9, not listed in
    // OutputSink::showMenu's printed text) for seeding demo data
    // (PLAN.md Phase 6).
    MainMenuController(InputSource& input, OutputSink& output, SummaryProvider& summaryProvider,
                        SubController& sampleManagement, SubController& orderIntake,
                        SubController& orderApproval, SubController& monitoring,
                        SubController& productionLine, SubController& shipment,
                        SubController& dummyData);

    void run();

private:
    InputSource& input_;
    OutputSink& output_;
    SummaryProvider& summaryProvider_;
    SubController& sampleManagement_;
    SubController& orderIntake_;
    SubController& orderApproval_;
    SubController& monitoring_;
    SubController& productionLine_;
    SubController& shipment_;
    SubController& dummyData_;
};

}  // namespace sos::controller
