#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "redis/core/ast.hpp"
namespace redis {
struct Redis : cursiv::GrammarSpec {
  struct SPACE : terminal<some(cr<R"( \t\r\v\f)">)> {};
  struct NEWLINE : terminal<lit<"\n">> {};
  static constexpr auto quoteEnd = !cr<R"(^ \t\r\n\v\f)">;
  static constexpr auto unquoted = cr<R"(^ \t\r\n'"\x00)">;
  static constexpr auto doubleQuoted =
      lit<"\""> + many((lit<"\\"> + cr<R"(^\n\x00)">) | cr<R"(^"\\\n\x00)">) +
      option(lit<"\"">);
  static constexpr auto singleQuoted =
      lit<"'"> + many(lit<"\\'"> | cr<R"(^'\n\x00)">) + option(lit<"'">);
  struct WORD
      : terminal<((some(unquoted) + option(doubleQuoted | singleQuoted)) |
                  doubleQuoted | singleQuoted) +
                 quoteEnd> {};
  struct ArgumentRule : rule<assign<&ast::Argument::raw>(ref<WORD>)> {};
  struct CommandRule
      : rule<assign<&ast::Command::name>(ref<ArgumentRule>) +
             many(append<&ast::Command::arguments>(ref<ArgumentRule>))> {};
  struct RootRule
      : rule<create<ast::Document>(
                 many(ref<NEWLINE>) +
                 option(some(append<&ast::Document::commands>(ref<CommandRule>),
                             some(ref<NEWLINE>)) +
                        many(ref<NEWLINE>))) +
             eof> {};
  using entry = RootRule;
  using ignore = SPACE;
};
} // namespace redis
