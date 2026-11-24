#include "json.h"

#include <charconv>

namespace json {

std::string Dump(const Value& value) {
  const auto text = glz::write_json(value);
  if (!text) throw std::runtime_error("cannot write the JSON value");
  return *text;
}

Value Parse(std::string_view text) {
  Value value;
  const auto error = glz::read_json(value, text);
  if (error) throw ParseError(glz::format_error(error, text));
  return value;
}

Value Array(std::initializer_list<Value> items) { return Value(Value::array_t(items)); }
Value Object() { return Value(Value::object_t{}); }

void Push(Value& array, Value item) { array.get_array().push_back(std::move(item)); }

const Value* Find(const Value& object, std::string_view key) {
  if (!object.is_object()) return nullptr;
  const auto& members = object.get_object();
  const auto found = members.find(key);
  return found == members.end() ? nullptr : &found->second;
}

std::string StringOr(const Value& object, std::string_view key, std::string fallback) {
  const Value* found = Find(object, key);
  return found != nullptr && found->is_string() ? found->get<std::string>() : std::move(fallback);
}

const Value::array_t& Items(const Value& value) {
  static const Value::array_t none;
  return value.is_array() ? value.get_array() : none;
}

namespace {

// A number is read back from its text: Glaze does not say which kind it holds, but it writes a whole
// number as digits only.
std::optional<std::int64_t> WholeNumber(const Value& value) {
  if (!value.is_number()) return std::nullopt;
  const std::string text = Dump(value);
  std::int64_t result = 0;
  const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result);
  if (parsed.ec != std::errc() || parsed.ptr != text.data() + text.size()) return std::nullopt;
  return result;
}

}  // namespace

std::optional<std::int64_t> AsInt64(const Value& value) { return WholeNumber(value); }

double AsDouble(const Value& value) {
  if (!value.is_number()) return 0;
  double result = 0;
  const std::string text = Dump(value);
  std::from_chars(text.data(), text.data() + text.size(), result);
  return result;
}

}  // namespace json

namespace glz {

bool operator==(const json::Value& a, const json::Value& b) {
  if (a.is_object()) {
    if (!b.is_object() || a.get_object().size() != b.get_object().size()) return false;
    for (const auto& [key, member] : a.get_object()) {
      const json::Value* other = json::Find(b, key);
      if (other == nullptr || !(member == *other)) return false;
    }
    return true;
  }
  if (a.is_array()) {
    if (!b.is_array() || a.get_array().size() != b.get_array().size()) return false;
    for (std::size_t i = 0; i < a.get_array().size(); ++i) {
      if (!(a.get_array()[i] == b.get_array()[i])) return false;
    }
    return true;
  }
  if (a.is_string()) return b.is_string() && a.get<std::string>() == b.get<std::string>();
  if (a.is_number()) return b.is_number() && json::Dump(a) == json::Dump(b);
  if (a.is_boolean()) return b.is_boolean() && a.get<bool>() == b.get<bool>();
  return a.is_null() && b.is_null();
}

}  // namespace glz
