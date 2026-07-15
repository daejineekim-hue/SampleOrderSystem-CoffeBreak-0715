#include "model/Order.h"

#include <stdexcept>

namespace sos::model {

std::string Order::statusToString(OrderStatus status) {
    switch (status) {
        case OrderStatus::RESERVED: return "RESERVED";
        case OrderStatus::REJECTED: return "REJECTED";
        case OrderStatus::PRODUCING: return "PRODUCING";
        case OrderStatus::CONFIRMED: return "CONFIRMED";
        case OrderStatus::RELEASE: return "RELEASE";
    }
    throw std::invalid_argument("Unknown OrderStatus");
}

OrderStatus Order::statusFromString(const std::string& text) {
    if (text == "RESERVED") return OrderStatus::RESERVED;
    if (text == "REJECTED") return OrderStatus::REJECTED;
    if (text == "PRODUCING") return OrderStatus::PRODUCING;
    if (text == "CONFIRMED") return OrderStatus::CONFIRMED;
    if (text == "RELEASE") return OrderStatus::RELEASE;
    throw std::invalid_argument("Unknown OrderStatus string: " + text);
}

json::JsonValue Order::toJson() const {
    json::JsonValue obj = json::JsonValue::makeObject();
    obj["orderId"] = json::JsonValue::makeString(orderId);
    obj["sampleId"] = json::JsonValue::makeString(sampleId);
    obj["customerName"] = json::JsonValue::makeString(customerName);
    obj["quantity"] = json::JsonValue::makeNumber(quantity);
    obj["status"] = json::JsonValue::makeString(statusToString(status));
    obj["createdAt"] = json::JsonValue::makeString(createdAt);
    return obj;
}

Order Order::fromJson(const json::JsonValue& value) {
    Order order;
    order.orderId = value.at("orderId").asString();
    order.sampleId = value.at("sampleId").asString();
    order.customerName = value.at("customerName").asString();
    order.quantity = static_cast<int>(value.at("quantity").asNumber());
    order.status = statusFromString(value.at("status").asString());
    order.createdAt = value.at("createdAt").asString();
    return order;
}

}  // namespace sos::model
