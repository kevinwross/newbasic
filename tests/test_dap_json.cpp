// M5: DAP JSON value (parse/dump) used by newbasic-dap.
#include <catch2/catch_test_macros.hpp>

#include <string>

#include "newbasic/dap/json.h"

using newbasic::dap::Json;

namespace {

Json ParseOk(const std::string& text) {
  Json json;
  std::string error;
  if (!Json::Parse(text, &json, &error)) {
    FAIL("Parse failed: " << error << " for [" << text << "]");
  }
  return json;
}

std::string ParseError(const std::string& text) {
  Json json;
  std::string error;
  CHECK_FALSE(Json::Parse(text, &json, &error));
  return error;
}

} // namespace

TEST_CASE("JSON: parses the DAP initialize request shape", "[dap][json]") {
  const Json json = ParseOk("{\"seq\":1,\"type\":\"request\",\"command\":\"initialize\","
                            "\"arguments\":{\"adapterID\":\"newbasic\",\"linesStartAt1\":true,"
                            "\"pathFormat\":\"path\"}}");
  REQUIRE(json.IsObject());
  CHECK(json.Find("seq")->AsNumber() == 1.0);
  CHECK(json.Find("command")->AsString() == "initialize");
  const Json* arguments = json.Find("arguments");
  REQUIRE(arguments != nullptr);
  CHECK(arguments->Find("adapterID")->AsString() == "newbasic");
  CHECK(arguments->Find("linesStartAt1")->AsBool());
  CHECK(json.Find("missing") == nullptr);
}

TEST_CASE("JSON: arrays, nesting, numbers and nulls", "[dap][json]") {
  const Json json =
      ParseOk("{\"lines\":[10,20,30],\"flags\":[],\"nested\":{\"a\":[{\"b\":null}]},\"n\":-1.5e2}");
  REQUIRE(json.Find("lines")->IsArray());
  REQUIRE(json.Find("lines")->Size() == 3);
  CHECK(json.Find("lines")->Items()[2].AsNumber() == 30.0);
  CHECK(json.Find("flags")->IsArray());
  CHECK(json.Find("flags")->Size() == 0);
  const Json* nested = json.Find("nested")->Find("a")->Items()[0].Find("b");
  REQUIRE(nested != nullptr);
  CHECK(nested->IsNull());
  CHECK(json.Find("n")->AsNumber() == -150.0);
}

TEST_CASE("JSON: string escapes, including \\u", "[dap][json]") {
  const Json json = ParseOk("{\"s\":\"a\\\"b\\\\c\\nd\\te\\u0041\\u00e9\"}");
  CHECK(json.Find("s")->AsString() == std::string("a\"b\\c\nd\te") + "A\xC3\xA9");
}

TEST_CASE("JSON: dump round-trips through parse", "[dap][json]") {
  Json request = Json::Object();
  request.Set("seq", Json::Number(2));
  request.Set("type", Json::Str("response"));
  request.Set("success", Json::Bool(true));
  Json body = Json::Object();
  body.Set("threadId", Json::Number(1));
  body.Set("allThreadsStopped", Json::Bool(true));
  request.Set("body", std::move(body));

  const std::string text = request.Dump();
  CHECK(text == "{\"seq\":2,\"type\":\"response\",\"success\":true,"
                "\"body\":{\"threadId\":1,\"allThreadsStopped\":true}}");
  const Json reparsed = ParseOk(text);
  CHECK(reparsed.Find("body")->Find("threadId")->AsNumber() == 1.0);
}

TEST_CASE("JSON: object member order is preserved and Set replaces", "[dap][json]") {
  Json object = Json::Object();
  object.Set("z", Json::Number(1));
  object.Set("a", Json::Number(2));
  object.Set("z", Json::Number(3)); // replaces in place
  CHECK(object.Dump() == "{\"z\":3,\"a\":2}");
}

TEST_CASE("JSON: malformed input reports a reason", "[dap][json]") {
  CHECK(ParseError("") == "unexpected end of input");
  CHECK(ParseError("{\"a\":}") == "unexpected character");
  CHECK(ParseError("{\"a\" 1}") == "expected ':'");
  CHECK(ParseError("[1,2") == "unterminated array");
  CHECK(ParseError("{\"a\":1}}") == "trailing characters");
  CHECK(ParseError("\"unterminated") == "unterminated string");
  CHECK(ParseError("tru") == "invalid literal");
  CHECK(ParseError("{\"a\":\"\\q\"}") == "invalid escape");
  CHECK(ParseError("{\"a\":\"\\u00\"}") == "invalid \\u escape");
  CHECK(ParseError("{\"a\":\"\\u0\"}") == "short \\u escape");
}

TEST_CASE("JSON: empty containers and unicode escapes in keys", "[dap][json]") {
  CHECK(ParseOk("{}").IsObject());
  CHECK(ParseOk("[]").IsArray());
  CHECK(ParseOk("{}").Dump() == "{}");
  CHECK(ParseOk("[]").Dump() == "[]");
  const Json json = ParseOk("{\"\\u0041\":1}");
  CHECK(json.Find("A")->AsNumber() == 1.0);
}
