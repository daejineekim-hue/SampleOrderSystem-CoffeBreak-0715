#pragma once

#include "controller/MainMenuController.h"

namespace sos::controller {

// Template-method skeleton for a console submenu: shows a numbered menu,
// reads a choice, dispatches to handle(), and loops until "0" (back).
// Removes the while(true)/cin>>choice/skipToNextLine boilerplate that used
// to be duplicated across SampleManagementController/OrderIntakeController/
// OrderApprovalController/ShipmentController (docs/design_refact.md item 3).
// MonitoringController/ProductionLineController/DummyDataController are
// single-shot screens rather than menus, so they don't use this.
class ConsoleSubmenuController : public SubController {
public:
    void run() final;

protected:
    // Prints the menu text; must end without a trailing newline (e.g. "선택 > ").
    virtual void showPrompt() = 0;

    // choice is never 0 (that's handled by run() as "back"). Return false
    // for an unrecognized choice; run() prints the standard error for it.
    virtual bool handle(int choice) = 0;
};

}  // namespace sos::controller
