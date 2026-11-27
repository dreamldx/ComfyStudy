#pragma once

#include <string>
#include <vector>

#include "json.h"

// JSON Schema, as far as the tool runtime needs it: what the model is told about the arguments of a tool
// (Normalize), and whether the arguments it sent fit (Validate).
//
// Validate knows a subset of the keywords: type, properties, required, additionalProperties (false),
// items, minItems, maxItems, enum, const, oneOf (only when every branch is a "const", which is how Glaze
// writes an enumeration), minimum, maximum, minLength, maxLength. The rest ($ref, anyOf, allOf, pattern,
// format, ...) is not checked; UncheckedKeywords says which ones a schema uses, so that the tool that
// brings the schema can be told.
namespace schema {

// The schema Glaze writes for a struct, made ready to be handed to a model: every "$ref" is replaced by
// the definition it points to, "$defs" and "title" are dropped, and a number type keeps only its "type"
// (Glaze lists the whole range of the C++ type as "minimum" and "maximum"); what is written next to a
// "$ref" (a description, a range of your own) stays and wins.
json::Value Normalize(const json::Value& schema);

// What is wrong with `value` as the arguments described by `schema`, one sentence per problem, each
// starting with the path of the value ("arguments", "arguments.ms", "arguments.items[2].name").
// An empty list means the value fits. Keywords that are not checked count as satisfied.
std::vector<std::string> Validate(const json::Value& schema, const json::Value& value);

// The keywords of `schema` (anywhere in it) that Validate does not check, sorted, each once.
// Annotations (title, description, default, examples, $schema, $comment) are not counted.
std::vector<std::string> UncheckedKeywords(const json::Value& schema);

}  // namespace schema
