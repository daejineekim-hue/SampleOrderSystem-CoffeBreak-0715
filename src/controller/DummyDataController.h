#pragma once

#include "controller/MainMenuController.h"
#include "generator/DummyDataGenerator.h"

namespace sos::controller {

// Hidden submenu (main menu choice 9, PLAN.md Phase 6) that seeds demo
// Sample data via DummyDataGenerator. Console rendering is out of
// unit-test scope, same as the other feature controllers.
class DummyDataController : public SubController {
public:
    explicit DummyDataController(generator::DummyDataGenerator& generator);

    void run() override;

private:
    generator::DummyDataGenerator& generator_;
};

}  // namespace sos::controller
