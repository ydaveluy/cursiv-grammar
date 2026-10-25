// The mckeemanform language: its grammar. McKeeman form has no ignored
// trivia -- every space and newline it reads is part of what it describes.
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "mckeemanform/core/ast.hpp"

namespace mckeemanform {

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct MckeemanForm : cursiv::GrammarSpec {
  struct NEWLINE
      : terminal<lit<"\r\n"> | lit<"\n"> | (lit<"\r"> + !lit<"\n">)> {};
  struct NAME : terminal<some(cr<"a-zA-Z_">)> {};
  struct INDENT : terminal<lit<"    ">> {};
  struct LEADING_WS : terminal<some(s | cr<"\u00a0\u1680\u2000-\u200a\u2028\u2029\u202f\u205f\u3000\ufeff">)> {};

  static constexpr auto hexDigit = cr<"0-9a-zA-Z">;
  static constexpr auto hexCode = (lit<"10"> + repeat<4>(hexDigit)) |
                                  (option(hexDigit) + repeat<4>(hexDigit));
  struct SINGLETON : terminal<lit<"'"> + (any | hexCode) + lit<"'">, Unquoted> {
  };
  struct STRING : terminal<through(lit<"\"">, lit<"\"">), Unquoted> {};

  struct RuleRefRule : rule<assign<&ast::RuleRef::target>(ref<NAME>)> {};

  static constexpr auto rangeFrom = assign<&ast::Range::from>(ref<SINGLETON>);
  static constexpr auto rangeTo =
      lit<" . "> + assign<&ast::Range::to>(ref<SINGLETON>);

  // The shape an exclusion names: a bare codepoint, or a closed range of two
  // -- never excludes of its own.
  struct ExclusionRule : rule<rangeFrom + option(rangeTo)> {};

  struct RangeRule
      : rule<rangeFrom +
             option(rangeTo + many(lit<" - "> + append<&ast::Range::excludes>(
                                                    ref<ExclusionRule>)))> {};

  static constexpr auto item = ref<RuleRefRule> | ref<RangeRule> | ref<STRING>;

  struct AlternativeRule
      : rule<ref<INDENT> +
             some(append<&ast::Alternative::items>(item) + option(lit<" ">)) +
             ref<NEWLINE>> {};

  struct ProductionRule
      : rule<assign<&ast::Production::name>(ref<NAME>) + ref<NEWLINE> +
             some(append<&ast::Production::alternatives>(
                 ref<AlternativeRule>))> {};

  struct DocumentRule
      : rule<option(ref<LEADING_WS>) +
             some(append<&ast::Document::rules>(ref<ProductionRule>) +
                  many(ref<NEWLINE>))> {};

  using entry = DocumentRule;
};

}  // namespace mckeemanform
