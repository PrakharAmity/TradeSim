// Small, dependency-free JSON value/parser used by TradeSim's local API.
#pragma once
#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace nlohmann {
class json {
public:
    using object_t = std::map<std::string, json>;
    using array_t = std::vector<json>;
    json() : value_(nullptr) {}
    json(std::nullptr_t) : value_(nullptr) {}
    json(bool v) : value_(v) {}
    json(int v) : value_(static_cast<double>(v)) {}
    json(long long v) : value_(static_cast<double>(v)) {}
    json(double v) : value_(v) {}
    json(const char* v) : value_(std::string(v)) {}
    json(const std::string& v) : value_(v) {}
    json(std::string&& v) : value_(std::move(v)) {}
    json(const object_t& v) : value_(v) {}
    json(const array_t& v) : value_(v) {}

    static json object() { return object_t{}; }
    static json array() { return array_t{}; }

    json& operator[](const std::string& key) {
        if (!std::holds_alternative<object_t>(value_)) value_ = object_t{};
        return std::get<object_t>(value_)[key];
    }
    void push_back(const json& value) {
        if (!std::holds_alternative<array_t>(value_)) value_ = array_t{};
        std::get<array_t>(value_).push_back(value);
    }

    template <typename T>
    T value(const std::string& key, const T& fallback) const {
        if (!std::holds_alternative<object_t>(value_)) return fallback;
        const auto& object = std::get<object_t>(value_);
        const auto it = object.find(key);
        if (it == object.end()) return fallback;
        const json& item = it->second;
        if constexpr (std::is_same_v<T, std::string>) {
            if (std::holds_alternative<std::string>(item.value_)) return std::get<std::string>(item.value_);
        } else if constexpr (std::is_same_v<T, bool>) {
            if (std::holds_alternative<bool>(item.value_)) return std::get<bool>(item.value_);
        } else if constexpr (std::is_arithmetic_v<T>) {
            if (std::holds_alternative<double>(item.value_)) return static_cast<T>(std::get<double>(item.value_));
        }
        return fallback;
    }

    std::string dump() const { std::ostringstream out; write(out); return out.str(); }

    static json parse(const std::string& input) {
        Parser parser(input);
        json result = parser.read_value();
        parser.skip_space();
        if (!parser.at_end()) throw std::runtime_error("Unexpected trailing JSON data");
        return result;
    }

private:
    using storage_t = std::variant<std::nullptr_t, bool, double, std::string, object_t, array_t>;
    storage_t value_;

    static void write_string(std::ostringstream& out, const std::string& value) {
        out << '"';
        for (unsigned char ch : value) {
            switch (ch) {
                case '"': out << "\\\""; break;
                case '\\': out << "\\\\"; break;
                case '\b': out << "\\b"; break;
                case '\f': out << "\\f"; break;
                case '\n': out << "\\n"; break;
                case '\r': out << "\\r"; break;
                case '\t': out << "\\t"; break;
                default: if (ch < 0x20) out << "?"; else out << static_cast<char>(ch);
            }
        }
        out << '"';
    }
    void write(std::ostringstream& out) const {
        if (std::holds_alternative<std::nullptr_t>(value_)) out << "null";
        else if (std::holds_alternative<bool>(value_)) out << (std::get<bool>(value_) ? "true" : "false");
        else if (std::holds_alternative<double>(value_)) {
            const double number = std::get<double>(value_);
            if (number == 0.0) out << '0';
            else out << std::setprecision(12) << number;
        } else if (std::holds_alternative<std::string>(value_)) write_string(out, std::get<std::string>(value_));
        else if (std::holds_alternative<object_t>(value_)) {
            out << '{'; bool first = true;
            for (const auto& entry : std::get<object_t>(value_)) {
                if (!first) out << ','; first = false;
                write_string(out, entry.first); out << ':'; entry.second.write(out);
            }
            out << '}';
        } else {
            out << '['; bool first = true;
            for (const auto& item : std::get<array_t>(value_)) {
                if (!first) out << ','; first = false; item.write(out);
            }
            out << ']';
        }
    }

    class Parser {
    public:
        explicit Parser(const std::string& input) : input_(input) {}
        bool at_end() const { return pos_ >= input_.size(); }
        void skip_space() { while (!at_end() && std::isspace(static_cast<unsigned char>(input_[pos_]))) ++pos_; }
        json read_value() {
            skip_space();
            if (at_end()) throw std::runtime_error("Unexpected end of JSON");
            const char c = input_[pos_];
            if (c == '{') return read_object();
            if (c == '[') return read_array();
            if (c == '"') return json(read_string());
            if (c == 't') { expect_word("true"); return json(true); }
            if (c == 'f') { expect_word("false"); return json(false); }
            if (c == 'n') { expect_word("null"); return json(nullptr); }
            return read_number();
        }
    private:
        const std::string& input_; std::size_t pos_ = 0;
        void expect(char c) {
            skip_space();
            if (at_end() || input_[pos_] != c) throw std::runtime_error("Malformed JSON");
            ++pos_;
        }
        void expect_word(const char* word) {
            while (*word) { if (at_end() || input_[pos_++] != *word++) throw std::runtime_error("Malformed JSON literal"); }
        }
        std::string read_string() {
            expect('"'); std::string result;
            while (!at_end()) {
                char c = input_[pos_++];
                if (c == '"') return result;
                if (c == '\\') {
                    if (at_end()) break;
                    c = input_[pos_++];
                    switch (c) {
                        case '"': case '\\': case '/': result.push_back(c); break;
                        case 'b': result.push_back('\b'); break; case 'f': result.push_back('\f'); break;
                        case 'n': result.push_back('\n'); break; case 'r': result.push_back('\r'); break; case 't': result.push_back('\t'); break;
                        default: throw std::runtime_error("Unsupported JSON escape");
                    }
                } else result.push_back(c);
            }
            throw std::runtime_error("Unterminated JSON string");
        }
        json read_object() {
            expect('{'); object_t result; skip_space(); if (!at_end() && input_[pos_] == '}') { ++pos_; return result; }
            for (;;) {
                skip_space(); if (at_end() || input_[pos_] != '"') throw std::runtime_error("Expected JSON object key");
                std::string key = read_string(); expect(':'); result[key] = read_value(); skip_space();
                if (!at_end() && input_[pos_] == '}') { ++pos_; break; } expect(',');
            }
            return result;
        }
        json read_array() {
            expect('['); array_t result; skip_space(); if (!at_end() && input_[pos_] == ']') { ++pos_; return result; }
            for (;;) { result.push_back(read_value()); skip_space(); if (!at_end() && input_[pos_] == ']') { ++pos_; break; } expect(','); }
            return result;
        }
        json read_number() {
            const std::size_t start = pos_;
            if (input_[pos_] == '-') ++pos_;
            while (!at_end() && (std::isdigit(static_cast<unsigned char>(input_[pos_])) || input_[pos_] == '.' || input_[pos_] == 'e' || input_[pos_] == 'E' || input_[pos_] == '+' || input_[pos_] == '-')) ++pos_;
            if (pos_ == start) throw std::runtime_error("Expected JSON value");
            return json(std::strtod(input_.substr(start, pos_ - start).c_str(), nullptr));
        }
    };
};
}  // namespace nlohmann
