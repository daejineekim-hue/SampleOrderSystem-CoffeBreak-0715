#include "controller/SampleManagementController.h"

#include <iostream>
#include <stdexcept>

#include "view/ConsoleMenuIO.h"

namespace sos::controller {

using model::Sample;
using view::skipToNextLine;

namespace {

void printSample(const Sample& sample) {
    std::cout << "  " << sample.id << " | " << sample.name << " | 평균생산시간=" << sample.avgProcessTimeMin
               << "분 | 수율=" << sample.yieldRate << " | 재고=" << sample.stock << "\n";
}

}  // namespace

SampleManagementController::SampleManagementController(repository::SampleRepository& sampleRepository)
    : sampleRepository_(sampleRepository) {}

void SampleManagementController::showPrompt() {
    std::cout << "\n-- 시료 관리 --\n [1] 등록  [2] 전체 조회  [3] 검색  [0] 뒤로\n선택 > ";
}

bool SampleManagementController::handle(int choice) {
    switch (choice) {
        case 1:
            registerSample();
            return true;
        case 2:
            listSamples();
            return true;
        case 3:
            searchSamples();
            return true;
        default:
            return false;
    }
}

void SampleManagementController::registerSample() {
    Sample sample;
    std::cout << "시료 ID: ";
    std::getline(std::cin, sample.id);
    std::cout << "시료명: ";
    std::getline(std::cin, sample.name);
    std::cout << "평균 생산시간(분): ";
    std::cin >> sample.avgProcessTimeMin;
    std::cout << "수율 (0 초과 1 이하): ";
    std::cin >> sample.yieldRate;
    std::cout << "초기 재고: ";
    std::cin >> sample.stock;
    skipToNextLine();

    try {
        sampleRepository_.create(sample);
        std::cout << "등록되었습니다.\n";
    } catch (const std::invalid_argument& e) {
        std::cout << "[오류] " << e.what() << "\n";
    }
}

void SampleManagementController::listSamples() {
    auto samples = sampleRepository_.findAll();
    if (samples.empty()) {
        std::cout << "등록된 시료가 없습니다.\n";
        return;
    }
    for (const auto& sample : samples) printSample(sample);
}

void SampleManagementController::searchSamples() {
    std::cout << "검색어: ";
    std::string keyword;
    std::getline(std::cin, keyword);

    auto results = sampleRepository_.search(keyword);
    if (results.empty()) {
        std::cout << "검색 결과가 없습니다.\n";
        return;
    }
    for (const auto& sample : results) printSample(sample);
}

}  // namespace sos::controller
