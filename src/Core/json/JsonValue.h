#pragma once

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace sos::json {

enum class JsonType { Null, Bool, Number, String, Array, Object };

// Minimal hand-rolled JSON value type: no external dependency is needed for
// this PoC's schema (flat objects/arrays of strings, numbers, bools).
// Object keys preserve insertion order to keep serialized output stable.
class JsonValue {
public:
    JsonValue();  // Null

    static JsonValue makeObject();
    static JsonValue makeArray();
    static JsonValue makeString(const std::string& value);
    static JsonValue makeNumber(double value);
    static JsonValue makeBool(bool value);

    JsonType type() const { return type_; }
    bool isNull() const { return type_ == JsonType::Null; }
    bool isObject() const { return type_ == JsonType::Object; }
    bool isArray() const { return type_ == JsonType::Array; }

    // Object access
    JsonValue& operator[](const std::string& key);
    const JsonValue& at(const std::string& key) const;
    bool contains(const std::string& key) const;

    // Array access
    void push_back(JsonValue value);
    std::vector<JsonValue>& items();
    const std::vector<JsonValue>& items() const;

    // Scalar access
    std::string asString() const;
    double asNumber() const;
    bool asBool() const;

    std::string dump() const;
    static JsonValue parse(const std::string& text);

private:
    JsonType type_;
    bool boolValue_ = false;
    double numberValue_ = 0.0;
    std::string stringValue_;
    std::vector<JsonValue> arrayValue_;
    std::vector<std::pair<std::string, JsonValue>> objectValue_;

    void dumpTo(std::string& out) const;
};

}  // namespace sos::json
