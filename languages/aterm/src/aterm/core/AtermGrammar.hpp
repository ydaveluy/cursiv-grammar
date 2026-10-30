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
  struct Term;
  static constexpr auto annotations =
      option(lit<"{"> +
             many(append<&ast::Term::annotations>(ref<Term>), lit<",">) +
             lit<"}">) +
      option(lit<":"> + assign<&ast::Term::typeAnnotation>(ref<Term>)) +
      option(flag<&ast::Term::resultAnnotation>(lit<"?">));
  static constexpr auto arguments =
      lit<"("> + many(append<&ast::Function::args>(ref<Term>), lit<",">) +
      lit<")">;
  struct Term
      : rule<create<ast::Function>(
                 ((assign<&ast::Function::afun>(ref<STRING> | ref<ID>) +
                   option(arguments)) |
                  arguments) +
                 annotations) |
             create<ast::Number>(assign<&ast::Number::raw>(ref<NUMBER>) +
                                 annotations) |
             create<ast::Placeholder>(
                 lit<"<"> + assign<&ast::Placeholder::inner>(ref<Term>) +
                 lit<">"> + annotations) |
             create<ast::List>(
                 lit<"["> +
                 many(append<&ast::List::items>(ref<Term>), lit<",">) +
                 lit<"]"> + annotations)> {};
  struct Doc : rule<assign<&ast::Doc::term>(ref<Term>) + eof> {};
  using entry = Doc;
  using ignored = WS;
};
} // namespace aterm
