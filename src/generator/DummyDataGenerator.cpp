#include "generator/DummyDataGenerator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>

namespace sos::generator {

using model::Sample;

namespace {

constexpr const char* kIdPrefix = "DUMMY-";
constexpr std::array<const char*, 6> kSampleNames = {
    "실리콘 웨이퍼-8인치", "GaN 에피텍셜-4인치",  "SiC 파워기판-6인치",
    "포토레지스트-PR7",     "산화막 웨이퍼-SiO2", "GaAs 에피웨이퍼-4인치",
};

}  // namespace

DummyDataGenerator::DummyDataGenerator(repository::SampleRepository& repository)
    : repository_(repository), rng_(std::random_device{}()) {}

int DummyDataGenerator::nextIdSuffix() const {
    int maxSuffix = 0;
    for (const auto& sample : repository_.findAll()) {
        if (sample.id.rfind(kIdPrefix, 0) != 0) continue;
        try {
            int suffix = std::stoi(sample.id.substr(std::string(kIdPrefix).size()));
            maxSuffix = std::max(maxSuffix, suffix);
        } catch (const std::exception&) {
            // Not a "DUMMY-###" id; ignore for numbering purposes.
        }
    }
    return maxSuffix + 1;
}

Sample DummyDataGenerator::generateOne(int idSuffix) {
    std::uniform_int_distribution<size_t> nameDist(0, kSampleNames.size() - 1);
    std::uniform_real_distribution<double> processTimeDist(1.0, 60.0);
    std::uniform_real_distribution<double> yieldDist(0.70, 0.99);
    std::uniform_int_distribution<int> stockDist(0, 1000);

    Sample sample;
    char idBuffer[16];
    std::snprintf(idBuffer, sizeof(idBuffer), "%s%03d", kIdPrefix, idSuffix);
    sample.id = idBuffer;
    sample.name = kSampleNames[nameDist(rng_)];
    sample.avgProcessTimeMin = std::round(processTimeDist(rng_) * 100.0) / 100.0;
    sample.yieldRate = std::round(yieldDist(rng_) * 100.0) / 100.0;
    sample.stock = stockDist(rng_);
    return sample;
}

std::vector<Sample> DummyDataGenerator::generate(int count) {
    std::vector<Sample> created;
    if (count <= 0) return created;

    created.reserve(count);
    int suffix = nextIdSuffix();
    for (int i = 0; i < count; ++i) {
        Sample sample = generateOne(suffix + i);
        created.push_back(repository_.create(sample));
    }
    return created;
}

}  // namespace sos::generator
