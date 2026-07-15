#pragma once

#include <string>

#include "json/JsonValue.h"

namespace sos::model {

enum class OrderStatus { RESERVED, REJECTED, PRODUCING, CONFIRMED, RELEASE };

struct Order {
    std::string orderId;
    std::string sampleId;
    std::string customerName;
    int quantity = 0;
    OrderStatus status = OrderStatus::RESERVED;
    std::string createdAt;  // "YYYY-MM-DD HH:MM:SS", local time

    json::JsonValue toJson() const;
    static Order fromJson(const json::JsonValue& value);

    static std::string statusToString(OrderStatus status);
    static OrderStatus statusFromString(const std::string& text);
};

}  // namespace sos::model
