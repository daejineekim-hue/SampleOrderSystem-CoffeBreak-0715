#pragma once

#include <random>
#include <vector>

#include "model/Sample.h"
#include "repository/SampleRepository.h"

namespace sos::generator {

// Demo/test seed data generator (PLAN.md Phase 6), ported from the
// DummyDataGenerator PoC. Generates random-but-valid Sample entries and
// appends them via SampleRepository::create (never overwrites existing
// data). IDs use a "DUMMY-###" prefix distinct from real app-entered ids
// (e.g. "SMP-001") so demo data is easy to tell apart, and numbering
// continues from the existing max DUMMY-### suffix so repeated runs never
// collide.
class DummyDataGenerator {
public:
    explicit DummyDataGenerator(repository::SampleRepository& repository);

    std::vector<model::Sample> generate(int count);

private:
    repository::SampleRepository& repository_;
    std::mt19937 rng_;

    int nextIdSuffix() const;
    model::Sample generateOne(int idSuffix);
};

}  // namespace sos::generator
