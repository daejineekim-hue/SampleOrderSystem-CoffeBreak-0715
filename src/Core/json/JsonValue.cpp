#include "json/JsonValue.h"

#include <cctype>
#include <cmath>
#include <sstream>

namespace sos::json {

namespace {

class Parser {
public:
    explicit Parser(const std::string& text) : text_(text) {}

    JsonValue parse() {
        skipWhitespace();
        JsonValue value = parseValue();
        skipWhitespace();
        return value;
    }

private:
    const std::string& text_;
    size_t pos_ = 0;

    char peek() const {
        if (pos_ >= text_.size()) {
            throw std::runtime_error("JSON parse error: unexpected end of input");
        }
        return text_[pos_];
    }

    char next() { return text_[pos_++]; }

    void skipWhitespace() {
        while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_]))) {
            ++pos_;
        }
    }

    void expect(char c) {
        if (peek() != c) {
            throw std::runtime_error(std::string("JSON parse error: expected '") + c + "'");
        }
        ++pos_;
    }

    JsonValue parseValue() {
        skipWhitespace();
        switch (peek()) {
            case '{':
                return parseObject();
            case '[':
                return parseArray();
            case '"':
                return JsonValue::makeString(parseRawString());
            case 't':
            case 'f':
                return parseBool();
            case 'n':
                return parseNull();
            default:
                return parseNumber();
        }
    }

    JsonValue parseObject() {
        JsonValue obj = JsonValue::makeObject();
        expect('{');
        skipWhitespace();
        if (peek() == '}') {
            next();
            return obj;
        }
        while (true) {
            skipWhitespace();
            std::string key = parseRawString();
            skipWhitespace();
            expect(':');
            JsonValue value = parseValue();
            obj[key] = value;
            skipWhitespace();
            char c = next();
            if (c == ',') continue;
            if (c == '}') break;
            throw std::runtime_error("JSON parse error: expected ',' or '}' in object");
        }
        return obj;
    }

    JsonValue parseArray() {
        JsonValue arr = JsonValue::makeArray();
        expect('[');
        skipWhitespace();
        if (peek() == ']') {
            next();
            return arr;
        }
        while (true) {
            JsonValue value = parseValue();
            arr.push_back(value);
            skipWhitespace();
            char c = next();
            if (c == ',') continue;
            if (c == ']') break;
            throw std::runtime_error("JSON parse error: expected ',' or ']' in array");
        }
        return arr;
    }

    std::string parseRawString() {
        expect('"');
        std::string result;
        while (true) {
            char c = next();
            if (c == '"') break;
            if (c == '\\') {
                char escaped = next();
                switch (escaped) {
                    case '"': result += '"'; break;
                    case '\\': result += '\\'; break;
                    case '/': result += '/'; break;
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    case 'r': result += '\r'; break;
                    default: result += escaped; break;
                }
            } else {
                result += c;
            }
        }
        return result;
    }

    JsonValue parseBool() {
        if (text_.compare(pos_, 4, "true") == 0) {
            pos_ += 4;
            return JsonValue::makeBool(true);
        }
        if (text_.compare(pos_, 5, "false") == 0) {
            pos_ += 5;
            return JsonValue::makeBool(false);
        }
        throw std::runtime_error("JSON parse error: invalid literal");
    }

    JsonValue parseNull() {
        if (text_.compare(pos_, 4, "null") == 0) {
            pos_ += 4;
            return JsonValue();
        }
        throw std::runtime_error("JSON parse error: invalid literal");
    }

    JsonValue parseNumber() {
        size_t start = pos_;
        if (peek() == '-') next();
        while (pos_ < text_.size() &&
               (std::isdigit(static_cast<unsigned char>(text_[pos_])) || text_[pos_] == '.' ||
                text_[pos_] == 'e' || text_[pos_] == 'E' || text_[pos_] == '+' ||
                text_[pos_] == '-')) {
            ++pos_;
        }
        std::string numberText = text_.substr(start, pos_ - start);
        if (numberText.empty()) {
            throw std::runtime_error("JSON parse error: invalid number");
        }
        return JsonValue::makeNumber(std::stod(numberText));
    }
};

void appendEscaped(std::string& out, const std::string& value) {
    out += '"';
    for (char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\t': out += "\\t"; break;
            case '\r': out += "\\r"; break;
            default: out += c; break;
        }
    }
    out += '"';
}

}  // namespace

JsonValue::JsonValue() : type_(JsonType::Null) {}

JsonValue JsonValue::makeObject() {
    JsonValue v;
    v.type_ = JsonType::Object;
    return v;
}

JsonValue JsonValue::makeArray() {
    JsonValue v;
    v.type_ = JsonType::Array;
    return v;
}

JsonValue JsonValue::makeString(const std::string& value) {
    JsonValue v;
    v.type_ = JsonType::String;
    v.stringValue_ = value;
    return v;
}

JsonValue JsonValue::makeNumber(double value) {
    JsonValue v;
    v.type_ = JsonType::Number;
    v.numberValue_ = value;
    return v;
}

JsonValue JsonValue::makeBool(bool value) {
    JsonValue v;
    v.type_ = JsonType::Bool;
    v.boolValue_ = value;
    return v;
}

JsonValue& JsonValue::operator[](const std::string& key) {
    if (type_ == JsonType::Null) {
        type_ = JsonType::Object;
    }
    if (type_ != JsonType::Object) {
        throw std::runtime_error("JsonValue::operator[] called on a non-object value");
    }
    for (auto& entry : objectValue_) {
        if (entry.first == key) return entry.second;
    }
    objectValue_.emplace_back(key, JsonValue());
    return objectValue_.back().second;
}

const JsonValue& JsonValue::at(const std::string& key) const {
    for (const auto& entry : objectValue_) {
        if (entry.first == key) return entry.second;
    }
    throw std::out_of_range("JsonValue::at: key not found: " + key);
}

bool JsonValue::contains(const std::string& key) const {
    for (const auto& entry : objectValue_) {
        if (entry.first == key) return true;
    }
    return false;
}

void JsonValue::push_back(JsonValue value) {
    if (type_ == JsonType::Null) {
        type_ = JsonType::Array;
    }
    if (type_ != JsonType::Array) {
        throw std::runtime_error("JsonValue::push_back called on a non-array value");
    }
    arrayValue_.push_back(std::move(value));
}

std::vector<JsonValue>& JsonValue::items() { return arrayValue_; }

const std::vector<JsonValue>& JsonValue::items() const { return arrayValue_; }

std::string JsonValue::asString() const { return stringValue_; }

double JsonValue::asNumber() const { return numberValue_; }

bool JsonValue::asBool() const { return boolValue_; }

void JsonValue::dumpTo(std::string& out) const {
    switch (type_) {
        case JsonType::Null:
            out += "null";
            break;
        case JsonType::Bool:
            out += boolValue_ ? "true" : "false";
            break;
        case JsonType::Number: {
            if (numberValue_ == static_cast<long long>(numberValue_)) {
                out += std::to_string(static_cast<long long>(numberValue_));
            } else {
                std::ostringstream oss;
                oss << numberValue_;
                out += oss.str();
            }
            break;
        }
        case JsonType::String:
            appendEscaped(out, stringValue_);
            break;
        case JsonType::Array: {
            out += '[';
            for (size_t i = 0; i < arrayValue_.size(); ++i) {
                if (i > 0) out += ',';
                arrayValue_[i].dumpTo(out);
            }
            out += ']';
            break;
        }
        case JsonType::Object: {
            out += '{';
            for (size_t i = 0; i < objectValue_.size(); ++i) {
                if (i > 0) out += ',';
                appendEscaped(out, objectValue_[i].first);
                out += ':';
                objectValue_[i].second.dumpTo(out);
            }
            out += '}';
            break;
        }
    }
}

std::string JsonValue::dump() const {
    std::string out;
    dumpTo(out);
    return out;
}

JsonValue JsonValue::parse(const std::string& text) {
    Parser parser(text);
    return parser.parse();
}

}  // namespace sos::json
