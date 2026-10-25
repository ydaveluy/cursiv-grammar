#pragma once
#include "aterm/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"
namespace aterm {
struct Aterm : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n\v\f)">)> {};
  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_+*$-">)> {};
  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                           lit<"\"">> {};
  static constexpr auto exponent = cr<"eE"> + option(cr<"+-">) + many(d);
  struct NUMBER : terminal<option(cr<"+-">) +
                           ((some(d) + option(lit<"."> + many(d))) |
                            (lit<"."> + many(d))) +
                           option(exponent)> {};
  struct TermRule;
  static constexpr auto annotations =
      option(lit<"{"> +
             many(append<&ast::Term::annotations>(ref<TermRule>), lit<",">) +
             lit<"}">) +
      option(lit<":"> + assign<&ast::Term::typeAnnotation>(ref<TermRule>)) +
      option(flag<&ast::Term::resultAnnotation>(lit<"?">));
  static constexpr auto arguments =
      lit<"("> + many(append<&ast::Function::args>(ref<TermRule>), lit<",">) +
      lit<")">;
  struct TermRule
      : rule<create<ast::Function>(
                 ((assign<&ast::Function::afun>(ref<STRING> | ref<ID>) +
                   option(arguments)) |
                  arguments) +
                 annotations) |
             create<ast::Number>(assign<&ast::Number::raw>(ref<NUMBER>) +
                                 annotations) |
             create<ast::Placeholder>(
                 lit<"<"> + assign<&ast::Placeholder::inner>(ref<TermRule>) +
                 lit<">"> + annotations) |
             create<ast::List>(
                 lit<"["> +
                 many(append<&ast::List::items>(ref<TermRule>), lit<",">) +
                 lit<"]"> + annotations)> {};
  struct DocRule : rule<assign<&ast::Doc::term>(ref<TermRule>) + eof> {};
  using entry = DocRule;
  using ignore = WS;
};
} // namespace aterm
