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
  struct String
      : rule<assign<&ast::String::raw>(ref<QUOTEDSTRING> | ref<MULTILINE>)> {};
  struct Number : rule<assign<&ast::Number::text>(ref<NUMBER>)> {};
  struct Tag : rule<assign<&ast::Tag::text>(ref<TAG>)> {};
  struct StringList
      : rule<lit<"["> +
             many(append<&ast::StringList::strings>(ref<String>),
                  lit<",">) +
             lit<"]">> {};
  struct Test;
  static constexpr auto argument =
      ref<StringList> | ref<String> | ref<Number> | ref<Tag>;
  static constexpr auto trailing_tests =
      append<&ast::Arguments::tests>(ref<Test>) |
      flag<&ast::Arguments::test_list>(
          lit<"("> +
          many(append<&ast::Arguments::tests>(ref<Test>), lit<",">) +
          lit<")">);
  struct Arguments : rule<create<ast::Arguments> +
                              many(append<&ast::Arguments::items>(argument)) +
                              option(trailing_tests)> {};
  struct Test : rule<assign<&ast::Test::name>(ref<IDENTIFIER>) +
                         assign<&ast::Test::arguments>(ref<Arguments>)> {};
  struct Command
      : rule<assign<&ast::Command::name>(ref<IDENTIFIER>) +
             assign<&ast::Command::arguments>(ref<Arguments>) +
             (lit<";"> |
              flag<&ast::Command::has_block>(
                  lit<"{"> +
                  many(append<&ast::Command::body>(ref<Command>)) +
                  lit<"}">))> {};
  struct Script
      : rule<create<ast::Script> +
             many(append<&ast::Script::commands>(ref<Command>))> {};
  using entry = Script;
  using ignored = list<WS, LINECOMMENT, BLOCKCOMMENT>;
};
} // namespace sieve
