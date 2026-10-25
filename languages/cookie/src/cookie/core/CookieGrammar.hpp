#pragma once

#include <string_view>

#include "cookie/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace cookie {

inline constexpr auto value_text = [](std::string_view text) {
  text.remove_prefix(1);
  const auto first = text.find_first_not_of(" \t");
  if (first == text.npos)
    return text.substr(text.size());
  text.remove_prefix(first);
  if (text.starts_with('"'))
    return text.substr(1, text.size() - 2);
  return text.substr(0, text.find_last_not_of(" \t") + 1);
};

struct Cookie : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct NAME : terminal<some(cr<R"(^=;" \t\r\n)">)> {};
  struct VALUE : terminal<lit<"="> + many(cr<R"(^;"\r\n)">),
                          cursiv::text_conv<value_text>> {};
  struct QUOTED_VALUE
      : terminal<lit<"="> + many(cr<R"( \t)">) + lit<"\""> +
                     many((lit<"\\"> + any) | cr<R"(^"\\\r\n)">) + lit<"\"">,
                 cursiv::text_conv<value_text>> {};

  struct PairRule
      : rule<assign<&ast::Pair::name>(ref<NAME>) +
             option(flag<&ast::Pair::quoted>(
                        assign<&ast::Pair::value>(ref<QUOTED_VALUE>)) |
                    assign<&ast::Pair::value>(ref<VALUE>))> {};

  struct DocRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::pairs>(ref<PairRule>) + option(lit<";">)) +
             eof> {};

  using entry = DocRule;
  using ignore = WS;
};

}  // namespace cookie
