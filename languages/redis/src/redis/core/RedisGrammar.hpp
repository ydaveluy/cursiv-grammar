#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "redis/core/ast.hpp"
namespace redis {
struct Redis : cursiv::GrammarSpec {
  struct SPACE : terminal<some(cr<R"( \t\r\v\f)">)> {};
  struct NEWLINE : terminal<lit<"\n">> {};
  static constexpr auto quoteEnd = !cr<R"(^ \t\r\n\v\f)">;
  static constexpr auto unquoted = cr<R"(^ \t\r\n'"\x00)">;
  static constexpr auto unquotedStart = !cr<R"(\v\f)"> + unquoted;
  static constexpr auto doubleQuoted =
      lit<"\""> + many((lit<"\\"> + cr<R"(^\n\x00)">) | cr<R"(^"\\\n\x00)">) +
      option(lit<"\"">);
  static constexpr auto singleQuoted =
      lit<"'"> + many(lit<"\\'"> | cr<R"(^'\n\x00)">) + option(lit<"'">);
  struct WORD
      : terminal<((unquotedStart + many(unquoted) + option(doubleQuoted | singleQuoted)) |
                  doubleQuoted | singleQuoted) +
                 quoteEnd> {};
  struct Argument : rule<assign<&ast::Argument::raw>(ref<WORD>)> {};
  struct Command
      : rule<assign<&ast::Command::name>(ref<Argument>) +
             many(append<&ast::Command::arguments>(ref<Argument>))> {};
  struct Root
      : rule<create<ast::Document>(
                 many(ref<NEWLINE>) +
                 option(some(append<&ast::Document::commands>(ref<Command>),
                             some(ref<NEWLINE>)) +
                        many(ref<NEWLINE>))) +
             eof> {};
  using entry = Root;
  using ignored = SPACE;
};
} // namespace redis
