// The useragent language: its grammar.
#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "useragent/core/ast.hpp"

namespace useragent {

inline constexpr auto unparenthesize = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Useragent : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t)">)> {};

  struct STRING : terminal<some(cr<R"(a-zA-Z0-9!#$%&'*+.^_`|~-)">)> {};

  struct COMMENT
      : terminal<as<unparenthesize>(
            lit<"("> + many(ref<COMMENT> | (lit<"\\"> + cr<R"(^\x00-\x08\x0a-\x1f\x7f)">) |
                             cr<R"(^()\\\x00-\x08\x0a-\x1f\x7f)">) +
            lit<")">)> {};

  struct Product
      : rule<assign<&ast::Product::name>(ref<STRING>) +
             option(gap::none + lit<"/"> + gap::none +
                    append<&ast::Product::version>(ref<STRING>)) +
             many(!gap::none + append<&ast::Product::comments>(ref<COMMENT>))> {};

  struct Doc
      : rule<some(append<&ast::Document::products>(ref<Product>), !gap::none) +
             option(eol)> {};

  using entry = Doc;
  using ignored = WS;
};

}  // namespace useragent
