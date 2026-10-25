#pragma once

#include <cstdio>
#include <string>
#include "cursiv/core/parser/Parser.hpp"
#include "cursiv/core/services/CoreLanguage.hpp"
#include "cursiv/test/ValidationHelper.hpp"
#include "cursiv/test/ValidationResult.hpp"
#include "tnt/core/CoreServices.hpp"
#include "tnt/core/ast.hpp"

namespace native_regressions {
inline bool run() {
  auto language = cursiv::create_language<tnt::TntCoreServices, tnt::TntSharedCoreServices, tnt::install_core_module>();
  bool ok = true;
  const auto check = [&](bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "TNT regression: %s\n", message); ok = false; }
  };
  namespace ast = tnt::ast;
  const std::string source = "~Aa:S(a+b)=SSa";
  const auto result = language.language.parser->parse(source);
  const auto* negated = cursiv::ast_cast<ast::Negation>(result.value);
  const auto* quantified = negated ? cursiv::ast_cast<ast::QuantifiedExpression>(negated->expression) : nullptr;
  const auto* equation = quantified ? cursiv::ast_cast<ast::Equation>(quantified->expression) : nullptr;
  const auto* successor = equation ? cursiv::ast_cast<ast::SuccessorExpression>(equation->left) : nullptr;
  check(result.accepted && !result.cst().recovered() && result.diagnostics.empty(), "formula prefixes and successor of a compound term");
  check(negated && negated->count == 1 && quantified && quantified->variable &&
        quantified->variable->letter == "a" && equation, "negation and quantifier scope over the equation");
  check(successor && successor->count == 1 && cursiv::ast_cast<ast::GroupedExpression>(successor->expression), "successor contains the whole term");
  for (const char* text : {
      "∀a′:!<Sa′=0∨Ea:a=0>", "~!~a=b", "<a=a∧b=b>", "<a=a⊃b=b>",
      "<a=a→b=b>", "<a=a]b=b>", "(a⋅b)=(a.b)", "SS(Sa+SSb)=SS(a*b)"}) {
    const auto parsed = language.language.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "native formula form");
  }
  const auto primes = language.language.parser->parse("a ′ ' = a");
  const auto* primeEquation = cursiv::ast_cast<ast::Equation>(primes.value);
  const auto* variable = primeEquation ? cursiv::ast_cast<ast::Variable>(primeEquation->left) : nullptr;
  check(variable && variable->primes == 2 && !primes.cst().recovered(), "ASCII and Unicode prime count across trivia");
  for (const auto& [text, code] : {
      std::pair{"f=f", "tnt.variable"}, std::pair{"a+b*c=a", "tnt.term"},
      std::pair{"(a)=a", "tnt.term"}, std::pair{"<a=a&Aa:a=a>", "tnt.binding"},
      std::pair{"<S(a+b)=c&Aa:a=a>", "tnt.binding"}}) {
    const auto parsed = language.language.parser->parse(text);
    check(parsed.accepted && !parsed.cst().recovered() && parsed.diagnostics.empty(), "invalid formulas retain their AST");
    const auto validation = cursiv::test::validationHelper(language.language, text);
    bool found = false;
    for (const auto& d : validation.diagnostics)
      if (d.code && d.code->isString() && d.code->string() == code) found = true;
    check(found, code);
  }
  return ok;
}
}  // namespace native_regressions
