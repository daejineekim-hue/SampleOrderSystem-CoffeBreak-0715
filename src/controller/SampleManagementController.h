#pragma once

#include "controller/MainMenuController.h"
#include "repository/SampleRepository.h"

namespace sos::controller {

// Console-driven submenu for docs/FEATURES/sample-management.md (register/
// list/search). Console rendering is out of unit-test scope per
// docs/FEATURES/main-menu.md's Testability note; the underlying business
// logic (SampleRepository/Sample) is already covered by
// SampleTest/SampleRepositoryTest.
class SampleManagementController : public SubController {
public:
    explicit SampleManagementController(repository::SampleRepository& sampleRepository);

    void run() override;

private:
    repository::SampleRepository& sampleRepository_;

    void registerSample();
    void listSamples();
    void searchSamples();
};

}  // namespace sos::controller
