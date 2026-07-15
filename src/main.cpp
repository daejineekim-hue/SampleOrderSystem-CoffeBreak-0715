#include "controller/DummyDataController.h"
#include "controller/MainMenuController.h"
#include "controller/MenuSummaryProvider.h"
#include "controller/MonitoringController.h"
#include "controller/OrderApprovalController.h"
#include "controller/OrderIntakeController.h"
#include "controller/ProductionLineController.h"
#include "controller/SampleManagementController.h"
#include "controller/ShipmentController.h"
#include "generator/DummyDataGenerator.h"
#include "production/ProductionLine.h"
#include "repository/OrderRepository.h"
#include "repository/SampleRepository.h"
#include "view/ConsoleMenuIO.h"

int main() {
    sos::repository::SampleRepository sampleRepository("data/samples.json");
    sos::repository::OrderRepository orderRepository("data/orders.json", sampleRepository);
    sos::production::ProductionLine productionLine(orderRepository, sampleRepository);

    sos::controller::SampleManagementController sampleManagement(sampleRepository);
    sos::controller::OrderIntakeController orderIntake(orderRepository);
    sos::controller::OrderApprovalController orderApproval(orderRepository, productionLine);
    sos::controller::MonitoringController monitoring(sampleRepository, orderRepository);
    sos::controller::ProductionLineController productionLineView(productionLine);
    sos::controller::ShipmentController shipment(orderRepository);
    sos::generator::DummyDataGenerator dummyDataGenerator(sampleRepository);
    sos::controller::DummyDataController dummyData(dummyDataGenerator);

    sos::controller::MenuSummaryProvider summaryProvider(sampleRepository, orderRepository);
    sos::view::ConsoleInputSource input;
    sos::view::ConsoleOutputSink output;

    sos::controller::MainMenuController mainMenu(input, output, summaryProvider, sampleManagement,
                                                  orderIntake, orderApproval, monitoring,
                                                  productionLineView, shipment, dummyData);
    mainMenu.run();
    return 0;
}
