// The lambda language: its grammar. Untyped lambda calculus -- a variable, an
// abstraction, or an application of two terms; parentheses are what make
// application unambiguous, so every application is fully parenthesized.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lambda/core/ast.hpp"

namespace lambda {

struct Lambda : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct VARIABLE : terminal<cr<"a-z"> + many(cr<"a-zA-Z0-9">)> {};

  struct Expression;
  struct Function;
  struct Application;

  struct Expression
      : rule<create<ast::Variable>(
                 assign<&ast::Variable::name>(ref<VARIABLE>)) |
             ref<Function> | ref<Application>> {};

  struct Function
      : rule<lit<"λ"> + assign<&ast::Function::param>(ref<VARIABLE>) +
             lit<"."> + assign<&ast::Function::body>(ref<Expression>)> {};

  struct Application
      : rule<lit<"("> + assign<&ast::Application::func>(ref<Expression>) +
             assign<&ast::Application::arg>(ref<Expression>) +
             lit<")">> {};

  using entry = Expression;
  using ignored = WS;
};

}  // namespace lambda
