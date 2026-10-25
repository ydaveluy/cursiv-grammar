// The useragent language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "useragent/core/ast.hpp"

namespace useragent {

inline constexpr auto unparenthesize = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unparenthesized = cursiv::text_conv<unparenthesize>;

struct Useragent : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct STRING : terminal<some(cr<R"(a-zA-Z0-9!#$%&'*+.^_`|~-)">)> {};

  struct COMMENT
      : terminal<lit<"("> +
                 many(ref<COMMENT> | (!lit<"("> + !lit<")"> + any)) +
                 lit<")">, Unparenthesized> {};

  struct ProductRule
      : rule<assign<&ast::Product::name>(ref<STRING>) + option(lit<"/"> +
             append<&ast::Product::version>(ref<STRING>)) +
             option(assign<&ast::Product::comment>(ref<COMMENT>))> {};

  struct DocRule
      : rule<some(append<&ast::Document::products>(ref<ProductRule>))> {};

  using entry = DocRule;
  using ignore = WS;
};

}  // namespace useragent
