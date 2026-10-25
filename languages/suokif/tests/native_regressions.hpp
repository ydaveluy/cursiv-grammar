#pragma once

#include <cstdio>
#include <string>
#include <utility>
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "suokif/core/CoreServices.hpp"
#include "suokif/core/ast.hpp"

namespace native_regressions {
inline bool run() {
  auto language = cursiv::create_language<suokif::SuokifCoreServices,
      suokif::SuokifSharedCoreServices, suokif::install_core_module>();
  namespace ast = suokif::ast;
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "SUO-KIF regression: %s\n", message); ok = false; }
  };
  const std::string source = R"((forall (?1 @_) (xor (P ?1) (Q @_))))";
  const auto parsed = language.language.parser->parse(source);
  const auto* doc = cursiv::ast_cast<ast::Document>(parsed.value);
  const auto* quantifier = doc && doc->sentences.size() == 1 ? cursiv::ast_cast<ast::List>(doc->sentences[0]) : nullptr;
  const auto* binder = quantifier && quantifier->items.size() == 3 ? cursiv::ast_cast<ast::List>(quantifier->items[1]) : nullptr;
  const auto* body = quantifier && quantifier->items.size() == 3 ? cursiv::ast_cast<ast::List>(quantifier->items[2]) : nullptr;
  const auto* first = binder && binder->items.size() == 2 ? cursiv::ast_cast<ast::Variable>(binder->items[0]) : nullptr;
  const auto* second = binder && binder->items.size() == 2 ? cursiv::ast_cast<ast::Variable>(binder->items[1]) : nullptr;
  const auto* connective = body && body->items.size() == 3 ? cursiv::ast_cast<ast::Word>(body->items[0]) : nullptr;
  check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "quantifiers and xor parse cleanly");
  check(first && first->name == "?1" && second && second->name == "@_", "quantified regular and row variables retain their spelling");
  check(connective && connective->name == "xor" && body->parent == quantifier && binder->parent == quantifier, "ordered logical arguments and parents");

  const auto values = language.language.parser->parse(R"((P 9007199254740993 1,25 -12e-300 "a  \\b λ" FooFn (FooFn A)))");
  const auto* valueDoc = cursiv::ast_cast<ast::Document>(values.value);
  const auto* relation = valueDoc && valueDoc->sentences.size() == 1 ? cursiv::ast_cast<ast::List>(valueDoc->sentences[0]) : nullptr;
  check(values.accepted && !values.cst().recovered() && values.diagnostics.empty() && relation && relation->items.size() == 7, "native term forms");
  if (relation && relation->items.size() == 7) {
    const auto* integer = cursiv::ast_cast<ast::Number>(relation->items[1]);
    const auto* decimal = cursiv::ast_cast<ast::Number>(relation->items[2]);
    const auto* exponent = cursiv::ast_cast<ast::Number>(relation->items[3]);
    const auto* string = cursiv::ast_cast<ast::Str>(relation->items[4]);
    check(integer && integer->value == "9007199254740993" && decimal && decimal->value == "1,25" && exponent && exponent->value == "-12e-300", "numbers preserve precision and syntax");
    check(string && string->literal == R"("a  \\b λ")", "string keeps raw spaces, backslashes and Unicode");
    check(cursiv::ast_cast<ast::Word>(relation->items[5]) && cursiv::ast_cast<ast::List>(relation->items[6]), "function names and applications stay distinct");
  }
  for (const char* text : {source.c_str(), "(equal (FooFn A) 1e99999999999999999999)", "(?pred @args)", "(P 1abc)", "(P \"line\nnext\")"}) {
    const auto validation = cursiv::test::validationHelper(language.language, text);
    check(validation.diagnostics.empty(), "native valid forms have no diagnostics");
  }
  for (const auto& [text, code] : {
      std::pair{"(not (P A) (Q B))", "suokif.arguments"},
      std::pair{"(forall () (P A))", "suokif.arguments"},
      std::pair{"(forall (A) (P A))", "suokif.variable"},
      std::pair{"(P and)", "suokif.term"},
      std::pair{"(FooFn A)", "suokif.sentence"},
      std::pair{"Atom", "suokif.sentence"}}) {
    const auto result = language.language.parser->parse(text);
    check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "invalid forms retain an AST for validation");
    const auto validation = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& diagnostic : validation.diagnostics)
      if (diagnostic.code && diagnostic.code->isString() && diagnostic.code->string() == code) found = true;
    check(found, code);
  }
  const auto legacy = cursiv::test::validationHelper(language.language, "(= A B)");
  check(legacy.diagnostics.size() == 1 && legacy.diagnostics[0].code &&
      legacy.diagnostics[0].code->isString() && legacy.diagnostics[0].code->string() == "suokif.equality", "legacy equality gets a runtime diagnostic");
  return ok;
}
}  // namespace native_regressions
