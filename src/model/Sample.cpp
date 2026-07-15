#include "model/Sample.h"

#include <stdexcept>

namespace sos::model {

void Sample::validateFields(const Sample& sample) {
    if (sample.id.empty()) {
        throw std::invalid_argument("Sample.id must not be empty");
    }
    if (sample.name.empty()) {
        throw std::invalid_argument("Sample.name must not be empty");
    }
    if (sample.avgProcessTimeMin <= 0) {
        throw std::invalid_argument("Sample.avgProcessTimeMin must be > 0");
    }
    if (sample.yieldRate <= 0 || sample.yieldRate > 1) {
        throw std::invalid_argument("Sample.yieldRate must be in (0, 1]");
    }
    if (sample.stock < 0) {
        throw std::invalid_argument("Sample.stock must be >= 0");
    }
}

json::JsonValue Sample::toJson() const {
    json::JsonValue obj = json::JsonValue::makeObject();
    obj["id"] = json::JsonValue::makeString(id);
    obj["name"] = json::JsonValue::makeString(name);
    obj["avgProcessTimeMin"] = json::JsonValue::makeNumber(avgProcessTimeMin);
    obj["yieldRate"] = json::JsonValue::makeNumber(yieldRate);
    obj["stock"] = json::JsonValue::makeNumber(stock);
    return obj;
}

Sample Sample::fromJson(const json::JsonValue& value) {
    Sample sample;
    sample.id = value.at("id").asString();
    sample.name = value.at("name").asString();
    sample.avgProcessTimeMin = value.at("avgProcessTimeMin").asNumber();
    sample.yieldRate = value.at("yieldRate").asNumber();
    sample.stock = static_cast<int>(value.at("stock").asNumber());
    return sample;
}

}  // namespace sos::model
