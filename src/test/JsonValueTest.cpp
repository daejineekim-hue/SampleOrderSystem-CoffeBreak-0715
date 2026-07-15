#include <gtest/gtest.h>

#include "json/JsonValue.h"

using sos::json::JsonValue;

// Regression coverage for the JsonValue implementation vendored from the
// DataPersistence PoC, now exercised through GoogleTest in this project.

TEST(JsonValue, RoundTripsObjectWithScalarFields) {
    JsonValue obj = JsonValue::makeObject();
    obj["id"] = JsonValue::makeString("S-001");
    obj["stock"] = JsonValue::makeNumber(480);
    obj["active"] = JsonValue::makeBool(true);

    JsonValue parsed = JsonValue::parse(obj.dump());

    EXPECT_EQ(parsed.at("id").asString(), "S-001");
    EXPECT_EQ(parsed.at("stock").asNumber(), 480);
    EXPECT_TRUE(parsed.at("active").asBool());
}

TEST(JsonValue, RoundTripsArrayOfObjects) {
    JsonValue arr = JsonValue::makeArray();
    JsonValue item = JsonValue::makeObject();
    item["name"] = JsonValue::makeString("실리콘 웨이퍼-8인치");
    arr.push_back(item);

    JsonValue parsed = JsonValue::parse(arr.dump());

    ASSERT_EQ(parsed.items().size(), 1u);
    EXPECT_EQ(parsed.items()[0].at("name").asString(), "실리콘 웨이퍼-8인치");
}

TEST(JsonValue, ParseEmptyArrayAndObject) {
    EXPECT_EQ(JsonValue::parse("[]").items().size(), 0u);
    EXPECT_FALSE(JsonValue::parse("{}").contains("anything"));
}
