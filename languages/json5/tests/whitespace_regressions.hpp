#pragma once

#include <cstdio>
#include <string>
#include <string_view>
#include <variant>

#include "cursiv/core/parser/Parser.hpp"
#include "json5/core/ast.hpp"

namespace json5_whitespace_regressions {
inline bool run(const cursiv::Parser &parser) {
  // The JSON5 reference lexer accepts these outside strings and comments.
  for (const std::string_view whitespace :
       {"\t",     "\n",     "\v",     "\f",     "\r",     " ",      "\u00a0",
        "\u1680", "\u2000", "\u2001", "\u2002", "\u2003", "\u2004", "\u2005",
        "\u2006", "\u2007", "\u2008", "\u2009", "\u200a", "\u2028", "\u2029",
        "\u202f", "\u205f", "\u3000", "\ufeff"}) {
    const auto gap = std::string(whitespace);
    const auto source = "{" + gap + "key" + gap + ":" + gap + "1" + gap + "}";
    const auto result = parser.parse(source);
    const auto *doc = result.value_as<json5::ast::Doc>();
    const auto *root = doc && doc->value
                           ? std::get_if<json5::ast::Value *>(&*doc->value)
                           : nullptr;
    const auto *object =
        root ? cursiv::ast_cast<json5::ast::Object>(*root) : nullptr;
    const auto *member =
        object && object->members.size() == 1 ? object->members[0] : nullptr;
    const auto *value =
        member ? std::get_if<json5::ast::Value *>(&member->value) : nullptr;
    const auto *number =
        value ? cursiv::ast_cast<json5::ast::Number>(*value) : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !member || member->name != "key" ||
        !number || number->text != "1") {
      std::fputs("json5: whitespace lost a key or value\n", stderr);
      return false;
    }
  }
  for (const std::string_view newline : {"\u2028", "\u2029"}) {
    const auto result = parser.parse("// comment" + std::string(newline) + "1");
    const auto *doc = result.value_as<json5::ast::Doc>();
    const auto *root = doc && doc->value
                           ? std::get_if<json5::ast::Value *>(&*doc->value)
                           : nullptr;
    const auto *number =
        root ? cursiv::ast_cast<json5::ast::Number>(*root) : nullptr;
    if (!result.accepted || result.cst().recovered() ||
        !result.diagnostics.empty() || !number || number->text != "1")
      return false;
  }
  return true;
}
} // namespace json5_whitespace_regressions
