#pragma once

#include "controller/MainMenuController.h"
#include "production/ProductionLine.h"

namespace sos::controller {

// Console-driven read-only view for docs/FEATURES/production-line.md
// (current producing order + waiting queue). advanceIfDue() is applied
// internally by ProductionLine's own query methods.
class ProductionLineController : public SubController {
public:
    explicit ProductionLineController(production::ProductionLine& productionLine);

    void run() override;

private:
    production::ProductionLine& productionLine_;
};

}  // namespace sos::controller
