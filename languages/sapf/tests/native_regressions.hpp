#pragma once
#include <cmath>
#include <cstdio>
#include "cursiv/core/parser/Parser.hpp"
#include "sapf/core/ast.hpp"

inline bool sapf_native_regressions(const auto& language) {
  namespace ast = sapf::ast;
  bool ok = true;
  auto check = [&](bool condition, const char* message) {
    if (!condition) { std::fprintf(stderr, "sapf native regression: %s\n", message); ok = false; }
  };
  const auto result = language.language.parser->parse(
      "[] #[] () {} \"hello\" \\x1 [] = f1 { :rate1 2 } +1 1E+2 -3e-2 name2");
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(),
        "empty containers, string values, numbered words and signed/exponent numbers");
  const auto* file = result.template value_as<ast::File>();
  check(file && file->items.size() == 11, "all source items retained");
  if (!file || file->items.size() != 11) return false;
  const auto* list = cursiv::ast_cast<ast::ListLiteral>(file->items[0]);
  const auto* array = cursiv::ast_cast<ast::ListLiteral>(file->items[1]);
  const auto* group = cursiv::ast_cast<ast::Group>(file->items[2]);
  const auto* form = cursiv::ast_cast<ast::FormLiteral>(file->items[3]);
  const auto* string = cursiv::ast_cast<ast::StringLiteral>(file->items[4]);
  const auto* function = cursiv::ast_cast<ast::FunctionLiteral>(file->items[5]);
  const auto* keyed = cursiv::ast_cast<ast::FormLiteral>(file->items[6]);
  check(list && list->items.empty() && !list->hashPrefix && array && array->items.empty() &&
            array->hashPrefix && group && group->items.empty() && form && form->items.empty(),
        "empty container kinds remain distinct");
  check(string && string->value == "hello", "string value owns a typed source AST node");
  check(function && function->params.size() == 1 && function->params[0] == "x1" &&
            function->body && function->body->items.empty() && function->name &&
            function->name->name == "f1", "empty function body and numbered parameter/name");
  check(keyed && keyed->items.size() == 2 && std::holds_alternative<ast::Key*>(keyed->items[0]) &&
            std::get<ast::Key*>(keyed->items[0])->name == "rate1", "numbered form key retained");
  const auto* one = cursiv::ast_cast<ast::NumberLiteral>(file->items[7]);
  const auto* hundred = cursiv::ast_cast<ast::NumberLiteral>(file->items[8]);
  const auto* small = cursiv::ast_cast<ast::NumberLiteral>(file->items[9]);
  const auto* word = cursiv::ast_cast<ast::Word>(file->items[10]);
  check(one && one->value == 1 && hundred && hundred->value == 100 &&
            small && std::abs(small->value + .03) < 1e-12 && word && word->name == "name2",
        "number converters and full word spelling");
  for (const char* source : {"[", "{", "(", "\"unterminated"}) {
    const auto bad = language.language.parser->parse(source);
    check(!bad.accepted || bad.cst().recovered() || !bad.diagnostics.empty(), "unfinished container/string rejected");
  }
  return ok;
}
