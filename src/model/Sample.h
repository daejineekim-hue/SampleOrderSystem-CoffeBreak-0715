#pragma once

#include <string>

#include "json/JsonValue.h"

namespace sos::model {

struct Sample {
    std::string id;
    std::string name;
    double avgProcessTimeMin = 0.0;
    double yieldRate = 0.0;
    int stock = 0;

    // Throws std::invalid_argument if any field violates the rules in
    // docs/FEATURES/sample-management.md (id/name non-empty, 0 < yieldRate <= 1,
    // avgProcessTimeMin > 0, stock >= 0). Does not know about other samples
    // (duplicate-id checking is the repository's job).
    static void validateFields(const Sample& sample);

    json::JsonValue toJson() const;
    static Sample fromJson(const json::JsonValue& value);
};

}  // namespace sos::model
