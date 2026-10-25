#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sieve/core/ast.hpp"

namespace sieve {
struct Sieve : cursiv::GrammarSpec {
  struct WS : terminal<some(s)> {};
  struct LINECOMMENT : terminal<lit<"#"> + many(cr<R"(^\n)">)> {};
  struct BLOCKCOMMENT : terminal<through(lit<"/*">, lit<"*/">)> {};
  struct IDENTIFIER : terminal<cr<"A-Za-z_"> + many(cr<"A-Za-z0-9_">)> {};
  struct TAG : terminal<lit<":"> + ref<IDENTIFIER>> {};
  struct QUOTEDSTRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                 lit<"\"">> {};
  static constexpr auto line_end = lit<"\r\n"> | lit<"\n">;
  struct MULTILINE
      : terminal<cr<"tT"> + cr<"eE"> + cr<"xX"> + cr<"tT"> + lit<":"> +
                 many(cr<R"( \t)">) + option(lit<"\r">) +
                 option(lit<"#"> + many(cr<R"(^\n)">)) + lit<"\n"> +
                 many(!(lit<"."> + line_end) + many(cr<R"(^\n)">) + lit<"\n">) +
                 lit<"."> + line_end> {};
  struct NUMBER : terminal<some(d) + option(cr<"KMGkmg">)> {};
  struct StringRule
      : rule<assign<&ast::String::raw>(ref<QUOTEDSTRING> | ref<MULTILINE>)> {};
  struct NumberRule : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct TagRule : rule<assign<&ast::Tag::text>(ref<TAG>)> {};
  struct StringListRule
      : rule<lit<"["> +
             many(append<&ast::StringList::strings>(ref<StringRule>),
                  lit<",">) +
             lit<"]">> {};
  struct TestRule;
  static constexpr auto argument =
      ref<StringListRule> | ref<StringRule> | ref<NumberRule> | ref<TagRule>;
  static constexpr auto trailing_tests =
      append<&ast::Arguments::tests>(ref<TestRule>) |
      flag<&ast::Arguments::test_list>(
          lit<"("> +
          many(append<&ast::Arguments::tests>(ref<TestRule>), lit<",">) +
          lit<")">);
  struct ArgumentsRule : rule<create<ast::Arguments> +
                              many(append<&ast::Arguments::items>(argument)) +
                              option(trailing_tests)> {};
  struct TestRule : rule<assign<&ast::Test::name>(ref<IDENTIFIER>) +
                         assign<&ast::Test::arguments>(ref<ArgumentsRule>)> {};
  struct CommandRule
      : rule<assign<&ast::Command::name>(ref<IDENTIFIER>) +
             assign<&ast::Command::arguments>(ref<ArgumentsRule>) +
             (lit<";"> |
              flag<&ast::Command::has_block>(
                  lit<"{"> +
                  many(append<&ast::Command::body>(ref<CommandRule>)) +
                  lit<"}">))> {};
  struct ScriptRule
      : rule<create<ast::Script> +
             many(append<&ast::Script::commands>(ref<CommandRule>))> {};
  using entry = ScriptRule;
  using ignore = list<WS, LINECOMMENT, BLOCKCOMMENT>;
};
} // namespace sieve
