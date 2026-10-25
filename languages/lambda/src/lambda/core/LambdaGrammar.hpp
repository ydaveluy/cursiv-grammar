// The lambda language: its grammar. Untyped lambda calculus -- a variable, an
// abstraction, or an application of two terms; parentheses are what make
// application unambiguous, so every application is fully parenthesized.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lambda/core/ast.hpp"

namespace lambda {

struct Lambda : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};

  struct VARIABLE : terminal<cr<"a-z"> + many(cr<"a-zA-Z0-9">)> {};

  struct ExpressionRule;
  struct FunctionRule;
  struct ApplicationRule;

  struct ExpressionRule
      : rule<create<ast::Variable>(
                 assign<&ast::Variable::name>(ref<VARIABLE>)) |
             ref<FunctionRule> | ref<ApplicationRule>> {};

  struct FunctionRule
      : rule<lit<"λ"> + assign<&ast::Function::param>(ref<VARIABLE>) +
             lit<"."> + assign<&ast::Function::body>(ref<ExpressionRule>)> {};

  struct ApplicationRule
      : rule<lit<"("> + assign<&ast::Application::func>(ref<ExpressionRule>) +
             assign<&ast::Application::arg>(ref<ExpressionRule>) +
             lit<")">> {};

  using entry = ExpressionRule;
  using ignore = WS;
};

}  // namespace lambda
