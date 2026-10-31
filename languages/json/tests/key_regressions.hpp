#pragma once

#include <cstdio>
#include <cmath>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "json/core/CoreServices.hpp"
#include "json/core/JsonString.hpp"
#include "json/core/ast.hpp"

namespace json_key_regressions {
inline bool run() {
  const auto services = cursiv::create_language<json::JsonCoreServices,
                                                json::JsonSharedCoreServices,
                                                json::install_core_module>();
  struct Key {
    std::string_view escaped, equivalent, decoded;
  };
  for (const Key key :
       {Key{R"(\b)", R"(\u0008)", "\b"}, Key{R"(\f)", R"(\u000c)", "\f"},
        Key{R"(\n)", R"(\u000a)", "\n"}, Key{R"(\r)", R"(\u000d)", "\r"},
        Key{R"(\t)", R"(\u0009)", "\t"}, Key{R"(\")", R"(\u0022)", "\""},
        Key{R"(\\)", R"(\u005c)", "\\"}, Key{R"(\/)", "/", "/"},
        Key{R"(\u0061)", "a", "a"}, Key{R"(\u00e9)", "é", "é"},
        Key{R"(\uD83D\uDE00)", "😀", "😀"},
        Key{R"(\u0000)", R"(\u0000)", std::string_view("\0", 1)}}) {
    if (json::decode_string(key.escaped) != key.decoded ||
        json::decode_string(key.equivalent) != key.decoded)
      return false;
    const std::string source = "{\"" + std::string(key.escaped) + "\":1,\"" +
                               std::string(key.equivalent) + "\":2}";
    const auto result = services.language.parser->parse(source);
    const auto *doc = result.value_as<json::ast::Doc>();
    const auto *value =
        doc ? std::get_if<json::ast::Value *>(&doc->value) : nullptr;
    const auto *object =
        value ? cursiv::ast_cast<json::ast::Object>(*value) : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !object || object->members.size() != 2 ||
        object->members[0]->name != key.decoded ||
        object->members[1]->name != key.decoded)
      return false;
    const auto validation =
        cursiv::test::validationHelper(services.language, source);
    if (validation.diagnostics.size() != 2)
      return false;
  }
  // Distinct control characters must not collide with printable characters.
  for (const auto source : {R"({"\n":1," ":2})", R"({"\r":1,"r":2})",
                            R"({"\t":1,"t":2})", R"({"\n":1,"\r":2,"\t":3})",
                            R"({"a":1,"\\u0061":2})", R"({"\n":1,"\\n":2})"}) {
    const auto validation =
        cursiv::test::validationHelper(services.language, source);
    if (!validation.diagnostics.empty())
      return false;
  }
  for (const Key key : {Key{R"(\b\f\n\r\t\"\\\/\u00e9\uD83D\uDE00)", "", "\b\f\n\r\t\"\\/é😀"},
                        Key{R"(\u0000x)", "", std::string_view("\0x", 2)}}) {
    const std::string source = "\"" + std::string(key.escaped) + "\"";
    const auto result = services.language.parser->parse(source);
    const auto* doc = result.value_as<json::ast::Doc>();
    const auto* value = doc ? std::get_if<std::string_view>(&doc->value) : nullptr;
    if (!result.accepted || result.cst().recovered() || !value || *value != key.decoded)
      return false;
  }
  // RFC 8259 §8.1: raw string characters must be well-formed UTF-8.
  for (const auto& raw : {std::string("\x80", 1), std::string("\xc3", 1),
                         std::string("\xc0\xaf", 2), std::string("\xed\xa0\x80", 3),
                         std::string("\xf4\x90\x80\x80", 4)}) {
    const std::string source = "\"" + raw + "\"";
    const auto result = services.language.parser->parse(source);
    if (result.accepted && !result.cst().recovered() && result.diagnostics.empty())
      return false;
  }
  for (const auto& raw : {std::string("\x7f", 1), std::string("\xc2\x80", 2),
                         std::string("\xf4\x8f\xbf\xbf", 4)}) {
    const std::string source = "\"" + raw + "\"";
    const auto result = services.language.parser->parse(source);
    const auto* doc = result.value_as<json::ast::Doc>();
    const auto* value = doc ? std::get_if<std::string_view>(&doc->value) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !value || *value != raw)
      return false;
  }
  {
    const auto result = services.language.parser->parse(
        R"({"all":[null,false,true,-0.0,{"u":"😀"}]})");
    const auto* document = result.value_as<json::ast::Doc>();
    const auto* value = document ? std::get_if<json::ast::Value*>(&document->value) : nullptr;
    const auto* object = value ? cursiv::ast_cast<json::ast::Object>(*value) : nullptr;
    const auto* items = object && object->members.size() == 1
        ? std::get_if<json::ast::Value*>(&object->members[0]->value) : nullptr;
    const auto* array = items ? cursiv::ast_cast<json::ast::Array>(*items) : nullptr;
    if (!result.accepted || result.cst().recovered() || !result.diagnostics.empty() ||
        !array || array->items.size() != 5 ||
        !std::holds_alternative<std::monostate>(array->items[0]) ||
        !std::holds_alternative<bool>(array->items[1]) || std::get<bool>(array->items[1]) ||
        !std::holds_alternative<bool>(array->items[2]) || !std::get<bool>(array->items[2]) ||
        !std::holds_alternative<double>(array->items[3]) ||
        std::get<double>(array->items[3]) != 0 || !std::signbit(std::get<double>(array->items[3])) ||
        object->parent != document || object->members[0]->parent != object ||
        array->parent != object->members[0]) return false;
    const auto* nested = std::get_if<json::ast::Value*>(&array->items[4]);
    const auto* child = nested ? cursiv::ast_cast<json::ast::Object>(*nested) : nullptr;
    if (!child || child->parent != array || child->members.size() != 1 ||
        child->members[0]->parent != child || child->members[0]->name != "u" ||
        !std::holds_alternative<std::string_view>(child->members[0]->value) ||
        std::get<std::string_view>(child->members[0]->value) != "😀") return false;
  }
  return true;
}
} // namespace json_key_regressions
