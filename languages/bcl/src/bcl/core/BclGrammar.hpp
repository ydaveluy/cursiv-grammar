// The bcl language: binary combinatory logic terms, prefix-encoded over the
// alphabet {0,1}. There is no name resolution and no comment syntax.
#pragma once

#include "bcl/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bcl {

struct Bcl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct K : terminal<as<ast::Combinator::K>(lit<"00">)> {};
  struct S : terminal<as<ast::Combinator::S>(lit<"01">)> {};

  // Forward-declared so `term` and `Apply` can name each other: an
  // application's two operands are themselves terms.
  struct Apply;
  static constexpr auto term = ref<K> | ref<S> | ref<Apply>;

  struct Apply : rule<lit<"1"> + assign<&ast::Apply::left>(term) +
                          assign<&ast::Apply::right>(term)> {};

  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::terms>(term))> {};

  using entry = Document;
  using ignored = WS;
};

}  // namespace bcl
