#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "suokif/core/ast.hpp"

namespace suokif {
struct Suokif : cursiv::GrammarSpec {
  struct WHITE : terminal<some(cr<R"( \t\n\r\v\f)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  static constexpr auto wordchar = cr<"a-zA-Z0-9_-">;
  struct WORD : terminal<lit<"<=>"> | lit<"=>"> | lit<"="> |
      (cr<"a-zA-Z"> + many(wordchar))> {};
  struct VARIABLE : terminal<cr<"?@"> + some(wordchar)> {};
  struct STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {};
  struct NUMBER : terminal<option(lit<"-">) + some(d) +
      option(cr<".,"> + some(d)) + option(lit<"e"> + option(lit<"-">) + some(d))> {};

  struct Term;
  struct Word : rule<assign<&ast::Word::name>(ref<WORD>)> {};
  struct Variable : rule<assign<&ast::Variable::name>(ref<VARIABLE>)> {};
  struct String : rule<assign<&ast::Str::literal>(ref<STRING>)> {};
  struct Number : rule<assign<&ast::Number::value>(ref<NUMBER>)> {};
  struct List : rule<create<ast::List> + lit<"("> +
      many(append<&ast::List::items>(ref<Term>)) + lit<")">> {};
  struct Term : rule<ref<List> | ref<String> | ref<Number> |
                         ref<Variable> | ref<Word>> {};
  struct Doc : rule<create<ast::Document> +
      many(append<&ast::Document::sentences>(ref<Term>))> {};
  using entry = Doc;
  using ignored = list<WHITE, COMMENT>;
};
}  // namespace suokif
