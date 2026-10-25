// The bcl language: binary combinatory logic terms, prefix-encoded over the
// alphabet {0,1}. There is no name resolution and no comment syntax.
#pragma once

#include "bcl/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bcl {

struct Bcl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct K : terminal<lit<"00">, value<ast::Combinator::K>> {};
  struct S : terminal<lit<"01">, value<ast::Combinator::S>> {};

  // Forward-declared so `term` and `ApplyRule` can name each other: an
  // application's two operands are themselves terms.
  struct ApplyRule;
  static constexpr auto term = ref<K> | ref<S> | ref<ApplyRule>;

  struct ApplyRule : rule<lit<"1"> + assign<&ast::Apply::left>(term) +
                          assign<&ast::Apply::right>(term)> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::terms>(term))> {};

  using entry = DocumentRule;
  using ignore = WS;
};

}  // namespace bcl
