#pragma once

#include "controller/MainMenuController.h"
#include "production/ProductionLine.h"

namespace sos::controller {

// Console-driven read-only view for docs/FEATURES/production-line.md
// (current producing order + waiting queue). advanceIfDue() is applied
// internally by ProductionLine's own query methods.
//
// Deliberately does NOT end with a "press Enter to continue" prompt: this is
// only ever invoked from MainMenuController, which reads the next menu
// number via getline() (already consumes the trailing newline). A trailing
// cin.ignore() here would have nothing left to skip and would instead
// swallow the user's next real keystroke (see system-test.ps1 scenario 7).
class ProductionLineController : public SubController {
public:
    explicit ProductionLineController(production::ProductionLine& productionLine);

    void run() override;

private:
    production::ProductionLine& productionLine_;
};

}  // namespace sos::controller
