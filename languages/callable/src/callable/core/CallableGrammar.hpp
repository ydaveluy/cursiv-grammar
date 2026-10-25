// The callable language: its grammar. Every statement is a name applied to
// parenthesised arguments, each itself a string or a nested call.
#pragma once

#include "callable/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace callable {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Callable : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<cr<R"(\r\n)">> {};

  struct ID : terminal<some(cr<"A-Za-z-">)> {};

  // No escape mechanism: a quote cannot appear inside a string at all.
  struct STRING
      : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">, Unquoted> {};

  struct LineRule;
  static constexpr auto f_arg = ref<LineRule> | ref<STRING>;

  struct LineRule
      : rule<assign<&ast::Call::name>(ref<ID>) + lit<"("> +
             many(append<&ast::Call::args>(f_arg), lit<",">) + lit<")">> {};

  struct ProgramRule
      : rule<some(append<&ast::Program::lines>(ref<LineRule>),
                  some(ref<EOL>)) +
             many(ref<EOL>)> {};

  using entry = ProgramRule;
  using ignore = WS;
};

}  // namespace callable
