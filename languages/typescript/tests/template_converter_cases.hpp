#pragma once

#include <cstdio>
#include <string_view>
#include <utility>

#include "typescript/core/TypeScriptGrammar.hpp"
#include "cursiv/core/parser/Parser.hpp"

namespace typescript_template_converter_cases {

inline bool run(const cursiv::Parser& parser) {
  namespace ast = typescript::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* reason) {
    if (!value) { std::fprintf(stderr, "typescript template converter: %s\n", reason); ok = false; }
  };
  for (std::string_view text : {"", "text", "'", "``", "`a`", "`a${", "`a${value", "`\\"}) {
    const auto result = typescript::template_open_text(text);
    check(!result && result.error.kind == cursiv::DiagnosticKind::conversion_error && !result.error.message.empty(),
          "missing opener, incomplete escape or embedded unescaped delimiter returns an error");
  }
  for (const auto [text, expected] : {
           std::pair<std::string_view, std::string_view>{"`", ""},
           {"`text", "text"}, {"`$name", "$name"}, {"`line\nnext", "line\nnext"},
           {"`a\\`b", "a\\`b"}, {"`a\\${value", "a\\${value"},
           {"`a\\\r\nb", "a\\\r\nb"}}) {
    const auto result = typescript::template_open_text(text);
    check(result && *result.value == expected, "valid raw quasi text is retained, including empty and escaped values");
  }
  for (const std::string_view source : {"const value = `", "const value = `unterminated;",
                                       "const value = `x${", "const value = `x\\"}) {
    const auto result = parser.parse(source);
    check(!result.accepted || result.cst().recovered() || !result.diagnostics.empty(),
          "unterminated template or interpolation never parses clean");
  }
  for (const std::string_view source : {"const value=``;", "const value=`a${name}c`;"}) {
    const auto result = parser.parse(source);
    const auto* program = cursiv::ast_cast<ast::Program>(result.value);
    const auto* declaration = program && program->body.size() == 1 ? cursiv::ast_cast<ast::VarDecl>(program->body[0]) : nullptr;
    const auto* binding = declaration && declaration->declarations.size() == 1 ? declaration->declarations[0] : nullptr;
    const auto* value = binding ? cursiv::ast_cast<ast::TemplateLit>(binding->init) : nullptr;
    const bool empty = source == "const value=``;";
    const auto* interpolation = value && value->expressions.size() == 1 ? cursiv::ast_cast<ast::Ident>(value->expressions[0]) : nullptr;
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty() && value && value->parent == binding &&
              (empty ? value->quasis.size() == 1 && value->quasis[0].empty() && value->expressions.empty()
                     : value->quasis.size() == 2 && value->quasis[0] == "a" && value->quasis[1] == "c" &&
                       interpolation && interpolation->name == "name" && interpolation->parent == value),
          "empty and interpolated templates keep raw quasis and ownership");
  }
  return ok;
}

} // namespace typescript_template_converter_cases
