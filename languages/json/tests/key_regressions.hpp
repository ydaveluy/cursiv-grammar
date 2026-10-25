#pragma once

#include <cstdio>
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
  return true;
}
} // namespace json_key_regressions
