#include <gtest/gtest.h>

#include <deque>

#include "controller/MainMenuController.h"

// docs/FEATURES/main-menu.md AC-1..AC-7.

using sos::controller::InputSource;
using sos::controller::MainMenuController;
using sos::controller::MenuSummary;
using sos::controller::OutputSink;
using sos::controller::SubController;
using sos::controller::SummaryProvider;

namespace {

class ScriptedInput : public InputSource {
public:
    explicit ScriptedInput(std::vector<std::string> lines) : lines_(lines.begin(), lines.end()) {}

    std::string readLine() override {
        if (lines_.empty()) return "0";  // safety net: never hang the test loop
        std::string line = lines_.front();
        lines_.pop_front();
        return line;
    }

private:
    std::deque<std::string> lines_;
};

class SpySubController : public SubController {
public:
    void run() override { ++callCount; }
    int callCount = 0;
};

class FakeSummaryProvider : public SummaryProvider {
public:
    MenuSummary getSummary() override {
        ++callCount;
        return MenuSummary{};
    }
    int callCount = 0;
};

class RecordingOutput : public OutputSink {
public:
    void showMenu(const MenuSummary&) override { ++menuShownCount; }
    void showError(const std::string& message) override {
        ++errorCount;
        lastError = message;
    }
    void showGoodbye() override { ++goodbyeCount; }

    int menuShownCount = 0;
    int errorCount = 0;
    int goodbyeCount = 0;
    std::string lastError;
};

class MainMenuControllerTest : public ::testing::Test {
protected:
    void runWith(std::vector<std::string> inputLines) {
        input_ = std::make_unique<ScriptedInput>(std::move(inputLines));
        MainMenuController controller(*input_, output_, summary_, sampleManagement_, orderIntake_,
                                       orderApproval_, monitoring_, productionLine_, shipment_,
                                       dummyData_);
        controller.run();
    }

    std::unique_ptr<ScriptedInput> input_;
    RecordingOutput output_;
    FakeSummaryProvider summary_;
    SpySubController sampleManagement_;
    SpySubController orderIntake_;
    SpySubController orderApproval_;
    SpySubController monitoring_;
    SpySubController productionLine_;
    SpySubController shipment_;
    SpySubController dummyData_;
};

}  // namespace

TEST_F(MainMenuControllerTest, Choice1_CallsSampleManagementOnly) {
    runWith({"1", "0"});

    EXPECT_EQ(sampleManagement_.callCount, 1);
    EXPECT_EQ(orderIntake_.callCount, 0);
    EXPECT_EQ(orderApproval_.callCount, 0);
    EXPECT_EQ(monitoring_.callCount, 0);
    EXPECT_EQ(productionLine_.callCount, 0);
    EXPECT_EQ(shipment_.callCount, 0);
}

TEST_F(MainMenuControllerTest, Choices2To6_RouteToCorrespondingControllers) {
    runWith({"2", "3", "4", "5", "6", "0"});

    EXPECT_EQ(orderIntake_.callCount, 1);
    EXPECT_EQ(orderApproval_.callCount, 1);
    EXPECT_EQ(monitoring_.callCount, 1);
    EXPECT_EQ(productionLine_.callCount, 1);
    EXPECT_EQ(shipment_.callCount, 1);
    EXPECT_EQ(sampleManagement_.callCount, 0);
}

TEST_F(MainMenuControllerTest, OutOfRangeNumber_ShowsErrorAndDispatchesNothing) {
    runWith({"7", "0"});

    EXPECT_GE(output_.errorCount, 1);
    EXPECT_EQ(sampleManagement_.callCount, 0);
    EXPECT_EQ(orderIntake_.callCount, 0);
    EXPECT_EQ(orderApproval_.callCount, 0);
    EXPECT_EQ(monitoring_.callCount, 0);
    EXPECT_EQ(productionLine_.callCount, 0);
    EXPECT_EQ(shipment_.callCount, 0);
}

TEST_F(MainMenuControllerTest, NonNumericInput_ShowsErrorAndDispatchesNothing) {
    runWith({"abc", "0"});

    EXPECT_GE(output_.errorCount, 1);
    EXPECT_EQ(sampleManagement_.callCount, 0);
}

TEST_F(MainMenuControllerTest, RecoversAfterInvalidInput) {
    runWith({"abc", "1", "0"});

    EXPECT_EQ(sampleManagement_.callCount, 1);
}

TEST_F(MainMenuControllerTest, Zero_ExitsWithoutDispatchingAndShowsGoodbye) {
    runWith({"0"});

    EXPECT_EQ(sampleManagement_.callCount, 0);
    EXPECT_EQ(orderIntake_.callCount, 0);
    EXPECT_EQ(orderApproval_.callCount, 0);
    EXPECT_EQ(monitoring_.callCount, 0);
    EXPECT_EQ(productionLine_.callCount, 0);
    EXPECT_EQ(shipment_.callCount, 0);
    EXPECT_EQ(output_.goodbyeCount, 1);
}

TEST_F(MainMenuControllerTest, ReturnsToMainMenuAfterSubControllerRuns) {
    runWith({"1", "2", "0"});

    EXPECT_EQ(sampleManagement_.callCount, 1);
    EXPECT_EQ(orderIntake_.callCount, 1);
}

TEST_F(MainMenuControllerTest, Choice9_CallsDummyDataControllerOnly) {
    runWith({"9", "0"});

    EXPECT_EQ(dummyData_.callCount, 1);
    EXPECT_EQ(sampleManagement_.callCount, 0);
    EXPECT_EQ(orderIntake_.callCount, 0);
    EXPECT_EQ(orderApproval_.callCount, 0);
    EXPECT_EQ(monitoring_.callCount, 0);
    EXPECT_EQ(productionLine_.callCount, 0);
    EXPECT_EQ(shipment_.callCount, 0);
}

TEST_F(MainMenuControllerTest, SummaryQueriedOnEachMenuDisplay) {
    runWith({"1", "0"});

    // Menu is shown on entry and again after the sub-controller returns.
    EXPECT_EQ(output_.menuShownCount, 2);
    EXPECT_EQ(summary_.callCount, 2);
}
