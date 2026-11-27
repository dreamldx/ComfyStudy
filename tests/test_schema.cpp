#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "json.h"
#include "schema.h"

namespace {

using Json = json::Value;

Json Schema(const char* text) { return json::Parse(text); }

// All the problems, joined, so that a test can look for one sentence.
std::string Problems(const char* def, const char* value) {
  std::string all;
  for (const std::string& problem : schema::Validate(Schema(def), json::Parse(value))) all += problem + "\n";
  return all;
}

bool Fits(const char* def, const char* value) { return schema::Validate(Schema(def), json::Parse(value)).empty(); }

constexpr const char* kSleepSchema = R"({"type":"object","properties":{"ms":{"type":"integer","minimum":0,"maximum":60000}},"required":["ms"]})";

TEST(SchemaValidate, AValidObjectFits) {
  EXPECT_TRUE(Fits(kSleepSchema, R"({"ms":100})"));
  EXPECT_TRUE(Fits(kSleepSchema, R"({"ms":0})"));
  EXPECT_TRUE(Fits(kSleepSchema, R"({"ms":60000})"));
}

TEST(SchemaValidate, ARequiredFieldThatIsMissing) {
  EXPECT_NE(Problems(kSleepSchema, "{}").find("arguments: missing required field \"ms\""), std::string::npos);
}

TEST(SchemaValidate, TheWrongType) {
  EXPECT_NE(Problems(kSleepSchema, R"({"ms":"soon"})").find("arguments.ms: must be an integer (got a string)"), std::string::npos);
  EXPECT_NE(Problems(kSleepSchema, R"({"ms":1.5})").find("arguments.ms: must be an integer (got a number)"), std::string::npos);
  EXPECT_NE(Problems(kSleepSchema, "[]").find("arguments: must be an object (got an array)"), std::string::npos);
}

TEST(SchemaValidate, ANumberOutsideTheRange) {
  EXPECT_NE(Problems(kSleepSchema, R"({"ms":70000})").find("arguments.ms: must be at most 60000 (got 70000)"), std::string::npos);
  EXPECT_NE(Problems(kSleepSchema, R"({"ms":-1})").find("arguments.ms: must be at least 0 (got -1)"), std::string::npos);
}

TEST(SchemaValidate, EveryProblemIsListedNotOnlyTheFirst) {
  const char* def = R"({"type":"object","properties":{"a":{"type":"number"},"b":{"type":"number"}},"required":["a","b"]})";
  const std::string all = Problems(def, R"({"a":"x"})");
  EXPECT_NE(all.find("arguments.a: must be a number"), std::string::npos);
  EXPECT_NE(all.find("missing required field \"b\""), std::string::npos);
}

TEST(SchemaValidate, AFieldThatIsNotDescribedIsRefusedWhenAdditionalPropertiesIsFalse) {
  const char* def = R"({"type":"object","properties":{"a":{"type":"number"}},"additionalProperties":false})";
  EXPECT_NE(Problems(def, R"({"a":1,"extra":true})").find("arguments: unexpected field \"extra\""), std::string::npos);
  EXPECT_TRUE(Fits(R"({"type":"object","properties":{"a":{"type":"number"}}})", R"({"a":1,"extra":true})"));
}

TEST(SchemaValidate, ANumberTypeAcceptsWholeNumbersToo) {
  EXPECT_TRUE(Fits(R"({"type":"number"})", "3"));
  EXPECT_TRUE(Fits(R"({"type":"number"})", "3.5"));
  EXPECT_TRUE(Fits(R"({"type":"integer"})", "9223372036854775807"));
  EXPECT_TRUE(Fits(R"({"type":"integer"})", "18446744073709551615"));
}

TEST(SchemaValidate, ATypeThatIsAList) {
  const char* def = R"({"type":["string","null"]})";
  EXPECT_TRUE(Fits(def, R"("text")"));
  EXPECT_TRUE(Fits(def, "null"));
  EXPECT_NE(Problems(def, "3").find("must be a string or null (got a number)"), std::string::npos);
}

TEST(SchemaValidate, TheItemsOfAnArray) {
  const char* def = R"({"type":"array","items":{"type":"object","properties":{"name":{"type":"string"}},"required":["name"]},"minItems":1,"maxItems":2})";
  EXPECT_TRUE(Fits(def, R"([{"name":"a"}])"));
  EXPECT_NE(Problems(def, R"([{"name":"a"},{"name":3}])").find("arguments[1].name: must be a string (got a number)"), std::string::npos);
  EXPECT_NE(Problems(def, "[]").find("arguments: must have at least 1 item(s) (got 0)"), std::string::npos);
  EXPECT_NE(Problems(def, R"([{"name":"a"},{"name":"b"},{"name":"c"}])").find("arguments: must have at most 2 item(s) (got 3)"), std::string::npos);
}

TEST(SchemaValidate, APathInsideAnObjectInsideAnArray) {
  const char* def = R"({"type":"object","properties":{"items":{"type":"array","items":{"type":"object","properties":{"name":{"type":"string"}}}}}})";
  EXPECT_NE(Problems(def, R"({"items":[{"name":"a"},{"name":"b"},{"name":7}]})").find("arguments.items[2].name: must be a string"), std::string::npos);
}

TEST(SchemaValidate, AnEnumerationWrittenAsEnumOrAsGlazeDoes) {
  const char* plain = R"({"type":"string","enum":["fast","slow"]})";
  const char* glaze = R"({"type":"string","oneOf":[{"title":"fast","const":"fast"},{"title":"slow","const":"slow"}]})";
  for (const char* def : {plain, glaze}) {
    EXPECT_TRUE(Fits(def, R"("fast")"));
    EXPECT_NE(Problems(def, R"("zzz")").find("must be one of \"fast\", \"slow\" (got \"zzz\")"), std::string::npos) << def;
  }
}

TEST(SchemaValidate, StringLengthCountsCharactersNotBytes) {
  const char* def = R"({"type":"string","minLength":2,"maxLength":3})";
  EXPECT_TRUE(Fits(def, "\"你好\""));  // two characters, six bytes
  EXPECT_NE(Problems(def, "\"你\"").find("must have at least 2 character(s) (got 1)"), std::string::npos);
  EXPECT_NE(Problems(def, "\"abcd\"").find("must have at most 3 character(s) (got 4)"), std::string::npos);
}

TEST(SchemaValidate, AConstantValue) {
  EXPECT_TRUE(Fits(R"({"const":"x"})", R"("x")"));
  EXPECT_NE(Problems(R"({"const":"x"})", R"("y")").find("must be \"x\" (got \"y\")"), std::string::npos);
}

TEST(SchemaValidate, KeywordsThatAreNotCheckedLetEverythingThrough) {
  EXPECT_TRUE(Fits(R"({"anyOf":[{"type":"string"},{"type":"number"}]})", "true"));
  EXPECT_TRUE(Fits(R"({"type":"string","pattern":"^a"})", R"("b")"));
}

TEST(SchemaUnchecked, ListsTheKeywordsThatAreNotChecked) {
  const Json def = Schema(R"({"type":"object","properties":{"a":{"anyOf":[{"type":"string"}],"description":"x"},
                                  "b":{"type":"string","pattern":"^a","format":"email"}},"$defs":{},"title":"T"})");
  EXPECT_EQ(schema::UncheckedKeywords(def), (std::vector<std::string>{"$defs", "anyOf", "format", "pattern"}));
}

TEST(SchemaUnchecked, AnEnumerationInGlazesFormIsChecked) {
  EXPECT_TRUE(schema::UncheckedKeywords(Schema(R"({"type":"string","oneOf":[{"const":"a"},{"const":"b"}]})")).empty());
  EXPECT_EQ(schema::UncheckedKeywords(Schema(R"({"oneOf":[{"type":"string"},{"type":"number"}]})")), (std::vector<std::string>{"oneOf"}));
}

TEST(SchemaNormalize, ReplacesReferencesAndDropsWhatTheModelDoesNotNeed) {
  // What Glaze writes for struct { double a; int ms; }, with a range and a description on ms.
  const Json written = Schema(R"({"type":"object","properties":{
      "a":{"$ref":"#/$defs/double","description":"the first number"},
      "ms":{"$ref":"#/$defs/int32_t","description":"how long","minimum":0,"maximum":60000}},
    "additionalProperties":false,
    "$defs":{"double":{"type":"number","minimum":-1.7976931348623157E308,"maximum":1.7976931348623157E308},
             "int32_t":{"type":"integer","minimum":-2147483648,"maximum":2147483647}},
    "required":["a","ms"],"title":"AddArgs"})");
  const Json expected = Schema(R"({"type":"object","properties":{
      "a":{"type":"number","description":"the first number"},
      "ms":{"type":"integer","description":"how long","minimum":0,"maximum":60000}},
    "additionalProperties":false,"required":["a","ms"]})");
  EXPECT_EQ(schema::Normalize(written), expected) << json::Dump(schema::Normalize(written));
}

TEST(SchemaNormalize, ADefinitionThatIsNotANumberIsKeptWhole) {
  const Json written = Schema(R"({"type":"object","properties":{"inner":{"$ref":"#/$defs/Inner"}},
    "$defs":{"Inner":{"type":"object","properties":{"n":{"$ref":"#/$defs/int32_t"}},"title":"Inner"},
             "int32_t":{"type":"integer","minimum":-2147483648,"maximum":2147483647}}})");
  const Json expected = Schema(R"({"type":"object","properties":{"inner":{"type":"object","properties":{"n":{"type":"integer"}}}}})");
  EXPECT_EQ(schema::Normalize(written), expected) << json::Dump(schema::Normalize(written));
}

TEST(SchemaNormalize, ASchemaWithoutReferencesComesBackAsItWas) {
  const Json plain = Schema(kSleepSchema);
  EXPECT_EQ(schema::Normalize(plain), plain);
}

}  // namespace
