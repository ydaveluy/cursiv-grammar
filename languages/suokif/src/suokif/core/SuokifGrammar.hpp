#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "suokif/core/ast.hpp"

namespace suokif {
struct Suokif : cursiv::GrammarSpec {
  struct WHITE : terminal<some(cr<R"( \t\n\r\v\f)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  static constexpr auto wordchar = cr<"a-zA-Z0-9_-">;
  struct WORD : terminal<lit<"<=>"> | lit<"=>"> | lit<"="> |
      cr<"a-zA-Z"> + many(wordchar)> {};
  struct VARIABLE : terminal<cr<"?@"> + some(wordchar)> {};
  struct STRING : terminal<lit<"\""> + many(cr<R"(^")">) + lit<"\"">> {};
  struct NUMBER : terminal<option(lit<"-">) + some(d) +
      option(cr<".,"> + some(d)) + option(lit<"e"> + option(lit<"-">) + some(d))> {};

  struct TermRule;
  struct WordRule : rule<assign<&ast::Word::name>(ref<WORD>)> {};
  struct VariableRule : rule<assign<&ast::Variable::name>(ref<VARIABLE>)> {};
  struct StringRule : rule<assign<&ast::Str::literal>(ref<STRING>)> {};
  struct NumberRule : rule<assign<&ast::Number::value>(ref<NUMBER>)> {};
  struct ListRule : rule<create<ast::List> + lit<"("> +
      many(append<&ast::List::items>(ref<TermRule>)) + lit<")">> {};
  struct TermRule : rule<ref<ListRule> | ref<StringRule> | ref<NumberRule> |
                         ref<VariableRule> | ref<WordRule>> {};
  struct DocRule : rule<create<ast::Document> +
      many(append<&ast::Document::sentences>(ref<TermRule>))> {};
  using entry = DocRule;
  using ignore = list<WHITE, COMMENT>;
};
}  // namespace suokif
