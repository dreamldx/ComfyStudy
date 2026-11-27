#include "schema.h"

#include <set>
#include <stdexcept>

namespace schema {
namespace {

using json::Value;

// --- Normalize ----------------------------------------------------------------------------------------

// Glaze names the definition of an arithmetic type after the C++ type and lists its whole range.
bool IsArithmeticDefinition(const std::string& name) {
  static const std::set<std::string> names = {"double",  "float",    "int",      "long",     "int8_t",   "int16_t",  "int32_t",
                                              "int64_t",  "uint8_t",  "uint16_t", "uint32_t", "uint64_t", "size_t",   "short",
                                              "unsigned", "char",     "ptrdiff_t"};
  return names.count(name) != 0;
}

constexpr int kMaxDepth = 32;

// `in_properties`: `node` is the "properties" object of a schema, whose keys are names of fields (a field may
// be called "title") and whose values are schemas.
Value Resolve(const Value& node, const Value& definitions, int depth, bool in_properties) {
  if (depth > kMaxDepth) throw std::runtime_error("the schema refers to itself, or is nested too deeply");
  if (node.is_array()) {
    Value out = json::Array();
    for (const Value& item : node.get_array()) json::Push(out, Resolve(item, definitions, depth + 1, false));
    return out;
  }
  if (!node.is_object()) return node;

  Value out = json::Object();
  const Value* reference = in_properties ? nullptr : json::Find(node, "$ref");
  if (reference != nullptr && reference->is_string()) {
    const std::string target = reference->get<std::string>();
    const std::string prefix = "#/$defs/";
    if (target.compare(0, prefix.size(), prefix) != 0) throw std::runtime_error("cannot resolve the reference " + target);
    const std::string name = target.substr(prefix.size());
    const Value* definition = json::Find(definitions, name);
    if (definition == nullptr) throw std::runtime_error("the schema has no definition called " + name);
    const Value resolved = Resolve(*definition, definitions, depth + 1, false);
    for (const auto& [key, member] : resolved.get_object()) {
      if (IsArithmeticDefinition(name) && key != "type") continue;  // the range of the C++ type is no information
      out.get_object()[key] = member;
    }
  }
  for (const auto& [key, member] : node.get_object()) {
    if (!in_properties && (key == "$ref" || key == "$defs" || key == "title")) continue;
    out.get_object()[key] = Resolve(member, definitions, depth + 1, !in_properties && key == "properties");
  }
  return out;
}

// --- Validate -----------------------------------------------------------------------------------------

bool IsWhole(const Value& number) {
  const std::string text = json::Dump(number);
  return text.find_first_of(".eE") == std::string::npos;
}

// The kind of a value as a word with its article: "a string", "an integer"...
std::string Describe(const Value& value) {
  if (value.is_object()) return "an object";
  if (value.is_array()) return "an array";
  if (value.is_string()) return "a string";
  if (value.is_boolean()) return "a boolean";
  if (value.is_number()) return "a number";
  return "null";
}

bool TypeMatches(const std::string& type, const Value& value) {
  if (type == "object") return value.is_object();
  if (type == "array") return value.is_array();
  if (type == "string") return value.is_string();
  if (type == "boolean") return value.is_boolean();
  if (type == "null") return value.is_null();
  if (type == "number") return value.is_number();
  if (type == "integer") return value.is_number() && IsWhole(value);
  return true;  // a type we do not know is not checked
}

std::string WithArticle(const std::string& type) {
  if (type == "object" || type == "array" || type == "integer") return "an " + type;
  if (type == "null") return type;
  return "a " + type;
}

std::size_t CountCharacters(const std::string& text) {
  std::size_t count = 0;
  for (const unsigned char c : text) {
    if ((c & 0xC0u) != 0x80u) ++count;  // not a continuation byte of UTF-8
  }
  return count;
}

// The values an enumeration allows: "enum", or "oneOf" whose every branch is a "const" (Glaze's way).
const Value::array_t* Choices(const Value& schema, std::vector<Value>& storage) {
  if (const Value* list = json::Find(schema, "enum"); list != nullptr && list->is_array()) return &list->get_array();
  const Value* branches = json::Find(schema, "oneOf");
  if (branches == nullptr || !branches->is_array() || branches->get_array().empty()) return nullptr;
  for (const Value& branch : branches->get_array()) {
    const Value* constant = json::Find(branch, "const");
    if (constant == nullptr) return nullptr;
    storage.push_back(*constant);
  }
  static thread_local Value::array_t holder;
  holder.assign(storage.begin(), storage.end());
  return &holder;
}

std::string Child(const std::string& path, const std::string& name) { return path + "." + name; }
std::string Index(const std::string& path, std::size_t i) { return path + "[" + std::to_string(i) + "]"; }

void Check(const Value& schema, const Value& value, const std::string& path, std::vector<std::string>& problems) {
  if (!schema.is_object()) return;

  if (const Value* type = json::Find(schema, "type")) {
    std::vector<std::string> types;
    if (type->is_string()) types.push_back(type->get<std::string>());
    if (type->is_array()) {
      for (const Value& t : type->get_array()) {
        if (t.is_string()) types.push_back(t.get<std::string>());
      }
    }
    bool matches = types.empty();
    for (const std::string& t : types) matches = matches || TypeMatches(t, value);
    if (!matches) {
      std::string wanted;
      for (std::size_t i = 0; i < types.size(); ++i) wanted += (i == 0 ? "" : " or ") + (types.size() == 1 ? WithArticle(types[i]) : (types[i] == "null" ? types[i] : "a " + types[i]));
      problems.push_back(path + ": must be " + wanted + " (got " + Describe(value) + ")");
      return;  // what follows assumes the type is right
    }
  }

  if (const Value* constant = json::Find(schema, "const")) {
    if (!(*constant == value)) problems.push_back(path + ": must be " + json::Dump(*constant) + " (got " + json::Dump(value) + ")");
  }
  std::vector<Value> storage;
  if (const Value::array_t* choices = Choices(schema, storage)) {
    bool found = false;
    std::string list;
    for (const Value& choice : *choices) {
      found = found || choice == value;
      list += (list.empty() ? "" : ", ") + json::Dump(choice);
    }
    if (!found) problems.push_back(path + ": must be one of " + list + " (got " + json::Dump(value) + ")");
  }

  if (value.is_number()) {
    if (const Value* minimum = json::Find(schema, "minimum"); minimum != nullptr && minimum->is_number()) {
      if (json::AsDouble(value) < json::AsDouble(*minimum)) problems.push_back(path + ": must be at least " + json::Dump(*minimum) + " (got " + json::Dump(value) + ")");
    }
    if (const Value* maximum = json::Find(schema, "maximum"); maximum != nullptr && maximum->is_number()) {
      if (json::AsDouble(value) > json::AsDouble(*maximum)) problems.push_back(path + ": must be at most " + json::Dump(*maximum) + " (got " + json::Dump(value) + ")");
    }
  }

  if (value.is_string()) {
    const std::size_t length = CountCharacters(value.get<std::string>());
    if (const Value* least = json::Find(schema, "minLength"); least != nullptr && least->is_number() && length < json::AsDouble(*least)) {
      problems.push_back(path + ": must have at least " + json::Dump(*least) + " character(s) (got " + std::to_string(length) + ")");
    }
    if (const Value* most = json::Find(schema, "maxLength"); most != nullptr && most->is_number() && length > json::AsDouble(*most)) {
      problems.push_back(path + ": must have at most " + json::Dump(*most) + " character(s) (got " + std::to_string(length) + ")");
    }
  }

  if (value.is_array()) {
    const std::size_t size = value.get_array().size();
    if (const Value* least = json::Find(schema, "minItems"); least != nullptr && least->is_number() && size < json::AsDouble(*least)) {
      problems.push_back(path + ": must have at least " + json::Dump(*least) + " item(s) (got " + std::to_string(size) + ")");
    }
    if (const Value* most = json::Find(schema, "maxItems"); most != nullptr && most->is_number() && size > json::AsDouble(*most)) {
      problems.push_back(path + ": must have at most " + json::Dump(*most) + " item(s) (got " + std::to_string(size) + ")");
    }
    if (const Value* items = json::Find(schema, "items"); items != nullptr && items->is_object()) {
      for (std::size_t i = 0; i < size; ++i) Check(*items, value.get_array()[i], Index(path, i), problems);
    }
  }

  if (value.is_object()) {
    if (const Value* required = json::Find(schema, "required"); required != nullptr && required->is_array()) {
      for (const Value& name : required->get_array()) {
        if (name.is_string() && json::Find(value, name.get<std::string>()) == nullptr) {
          problems.push_back(path + ": missing required field \"" + name.get<std::string>() + "\"");
        }
      }
    }
    const Value* properties = json::Find(schema, "properties");
    for (const auto& [key, member] : value.get_object()) {
      const Value* described = properties != nullptr ? json::Find(*properties, key) : nullptr;
      if (described != nullptr) {
        Check(*described, member, Child(path, key), problems);
      } else if (const Value* extra = json::Find(schema, "additionalProperties"); extra != nullptr && extra->is_boolean() && !extra->get<bool>()) {
        problems.push_back(path + ": unexpected field \"" + key + "\"");
      }
    }
  }
}

// --- UncheckedKeywords --------------------------------------------------------------------------------

void CollectUnchecked(const Value& schema, std::set<std::string>& found) {
  if (!schema.is_object()) return;
  static const std::set<std::string> checked = {"type", "required", "minItems", "maxItems", "enum", "const", "minimum", "maximum", "minLength", "maxLength"};
  static const std::set<std::string> notes = {"title", "description", "default", "examples", "$schema", "$comment", "$id"};
  std::vector<Value> storage;
  const bool enumeration = Choices(schema, storage) != nullptr;
  for (const auto& [key, member] : schema.get_object()) {
    if (checked.count(key) != 0 || notes.count(key) != 0) continue;
    if (key == "properties") {
      if (member.is_object()) {
        for (const auto& [name, sub] : member.get_object()) CollectUnchecked(sub, found);
      }
    } else if (key == "items") {
      if (member.is_object()) {
        CollectUnchecked(member, found);
      } else {
        found.insert(key);
      }
    } else if (key == "additionalProperties") {
      if (!member.is_boolean()) found.insert(key);
    } else if (key == "oneOf") {
      if (!enumeration) found.insert(key);
    } else {
      found.insert(key);
    }
  }
}

}  // namespace

Value Normalize(const Value& schema) {
  const Value* definitions = json::Find(schema, "$defs");
  const Value none = json::Object();
  return Resolve(schema, definitions != nullptr ? *definitions : none, 0, false);
}

std::vector<std::string> Validate(const Value& schema, const Value& value) {
  std::vector<std::string> problems;
  Check(schema, value, "arguments", problems);
  return problems;
}

std::vector<std::string> UncheckedKeywords(const Value& schema) {
  std::set<std::string> found;
  CollectUnchecked(schema, found);
  return {found.begin(), found.end()};
}

}  // namespace schema
