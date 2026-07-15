#include "repository/OrderRepository.h"

#include <algorithm>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace sos::repository {

using json::JsonValue;
using model::Order;
using model::OrderStatus;

namespace {

std::tm toLocalTm(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tmv{};
#ifdef _WIN32
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    return tmv;
}

std::string formatDate(std::chrono::system_clock::time_point tp) {
    std::tm tmv = toLocalTm(tp);
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y%m%d");
    return oss.str();
}

std::string formatDateTime(std::chrono::system_clock::time_point tp) {
    std::tm tmv = toLocalTm(tp);
    std::ostringstream oss;
    oss << std::put_time(&tmv, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

}  // namespace

OrderRepository::OrderRepository(std::string filePath, SampleRepository& sampleRepository,
                                  Clock clock)
    : filePath_(std::move(filePath)), sampleRepository_(sampleRepository), clock_(std::move(clock)) {
    load();
}

void OrderRepository::load() {
    orders_.clear();
    std::ifstream in(filePath_);
    if (!in.is_open()) return;

    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string text = buffer.str();
    if (text.empty()) return;

    JsonValue root = JsonValue::parse(text);
    for (const auto& item : root.items()) {
        orders_.push_back(Order::fromJson(item));
    }
}

void OrderRepository::save() const {
    JsonValue root = JsonValue::makeArray();
    for (const auto& order : orders_) {
        root.push_back(order.toJson());
    }
    std::ofstream out(filePath_, std::ios::trunc);
    out << root.dump();
}

bool OrderRepository::sampleExists(const std::string& sampleId) const {
    auto samples = sampleRepository_.findAll();
    return std::any_of(samples.begin(), samples.end(),
                        [&](const auto& s) { return s.id == sampleId; });
}

std::string OrderRepository::nextOrderId(const std::string& datePart) const {
    const std::string prefix = "ORD-" + datePart + "-";
    int maxSeq = 0;
    for (const auto& order : orders_) {
        if (order.orderId.rfind(prefix, 0) == 0) {
            int seq = std::stoi(order.orderId.substr(prefix.size()));
            maxSeq = std::max(maxSeq, seq);
        }
    }
    char seqBuf[8];
    std::snprintf(seqBuf, sizeof(seqBuf), "%04d", maxSeq + 1);
    return prefix + seqBuf;
}

const Order& OrderRepository::registerOrder(const std::string& sampleId,
                                             const std::string& customerName, int quantity) {
    if (sampleId.empty() || !sampleExists(sampleId)) {
        throw std::invalid_argument("등록되지 않은 시료 ID입니다: " + sampleId);
    }
    if (quantity <= 0) {
        throw std::invalid_argument("수량은 1 이상의 양의 정수여야 합니다");
    }
    if (trim(customerName).empty()) {
        throw std::invalid_argument("고객명을 입력해야 합니다");
    }

    auto now = clock_();
    Order order;
    order.orderId = nextOrderId(formatDate(now));
    order.sampleId = sampleId;
    order.customerName = customerName;
    order.quantity = quantity;
    order.status = OrderStatus::RESERVED;
    order.createdAt = formatDateTime(now);

    orders_.push_back(order);
    save();
    return orders_.back();
}

std::vector<Order> OrderRepository::findAll() const { return orders_; }

}  // namespace sos::repository
