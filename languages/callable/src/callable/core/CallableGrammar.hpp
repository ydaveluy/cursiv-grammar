// The callable language: its grammar. Every statement is a name applied to
// parenthesised arguments, each itself a string or a nested call.
#pragma once

#include "callable/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace callable {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Callable : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};
  struct EOL : terminal<cr<R"(\r\n)">> {};

  struct ID : terminal<some(cr<"A-Za-z-">)> {};

  // No escape mechanism: a quote cannot appear inside a string at all.
  struct STRING
      : terminal<as<unquote>(lit<"\""> + many(cr<R"(^")">) + lit<"\"">)> {};

  struct Line;
  static constexpr auto f_arg = ref<Line> | ref<STRING>;

  struct Line
      : rule<assign<&ast::Call::name>(ref<ID>) + lit<"("> +
             many(append<&ast::Call::args>(f_arg), lit<",">) + lit<")">> {};

  struct Program
      : rule<some(append<&ast::Program::lines>(ref<Line>),
                  some(ref<EOL>)) +
             many(ref<EOL>)> {};

  using entry = Program;
  using ignored = WS;
};

}  // namespace callable
