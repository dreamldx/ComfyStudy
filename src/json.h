#pragma once

#include <glaze/glaze.hpp>

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

// The JSON of the project: one value type for JSON whose shape is not known in advance (the
// messages of the three LLM services, the arguments of a tool call, a JSON Schema), and the few
// helpers around it that the rest of the code uses. The library is Glaze (glz::generic).
//
// What to know about it: a whole number is kept exactly (glz::generic_u64 stores a non-negative one as
// uint64_t and a negative one as int64_t; any other number is a double); an object keeps its members
// in the order they were read or added; and Glaze gives the value no operator==, which the tests need
// (see the end of this file).
namespace json {

using Value = glz::generic_u64;

// Thrown by Parse when the text is not valid JSON.
class ParseError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// The JSON text of a value, written without spaces.
std::string Dump(const Value& value);

// The value in `text`. Throws ParseError (the message says where it went wrong).
Value Parse(std::string_view text);

// An empty array ([]) and an empty object ({}).
Value Array(std::initializer_list<Value> items = {});
Value Object();

// Appends an item to an array (`array` must already be an array).
void Push(Value& array, Value item);

// The member `key` of an object, or nullptr when `object` is not an object or has no such member.
const Value* Find(const Value& object, std::string_view key);

// The string member `key` of an object, or `fallback` when there is none (or it is not a string).
std::string StringOr(const Value& object, std::string_view key, std::string fallback = "");

// The items of an array (an empty list when `value` is not an array).
const Value::array_t& Items(const Value& value);

// A whole number that fits a signed 64-bit integer, or nullopt for anything else (not a number, a
// fraction, or too big or too small).
std::optional<std::int64_t> AsInt64(const Value& value);

// Any number as a double (0 when `value` is not a number).
double AsDouble(const Value& value);

}  // namespace json

// Equality, for the tests: two values are equal when they have the same type and content; the order of
// the members of an object does not matter. It is in glz so that the assertions of GoogleTest find it.
namespace glz {
bool operator==(const json::Value& a, const json::Value& b);
}  // namespace glz
