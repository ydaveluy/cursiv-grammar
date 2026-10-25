#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "lcc/core/ast.hpp"

namespace lcc {

struct Lcc : cursiv::GrammarSpec {
  struct Letter : terminal<cr<"A-Za-z">> {};
  struct Letters : terminal<some(cr<"A-Za-z">)> {};
  struct Digits : terminal<some(d)> {};
  struct Punctuation : terminal<some(cr<R"(^A-Za-z0-9. \t\r\n)">)> {};
  struct CutterLetters : terminal<repeat<1, 3>(cr<"A-Za-z">) + !cr<"A-Za-z">> {};
  struct Year : terminal<repeat<4>(d) + !d> {};
  static constexpr auto SP = some(s);

  struct CutterRule
      : rule<assign<&ast::Cutter::letter>(ref<Letters>) + option(SP) +
             assign<&ast::Cutter::digits>(ref<Digits>)> {};

  static constexpr auto anotherCutter = &ref<CutterLetters> + ref<CutterRule>;
  static constexpr auto cuttersGroup = option(lit<".">) + option(SP) +
      append<&ast::CallNumber::cutters>(ref<CutterRule>) +
      many(option(SP) + append<&ast::CallNumber::cutters>(anotherCutter));

  struct EditionRule
      : rule<&ref<Year> + ref<Digits> + option(ref<Letters>),
             std::string_view> {};
  struct ItemRule
      : rule<some(ref<Letters> | ref<Digits> | lit<"."> | ref<Punctuation> | SP),
             std::string_view> {};

  struct CallNumberRule
      : rule<option(SP) + assign<&ast::CallNumber::topic>(ref<Letter>) +
             option(assign<&ast::CallNumber::subtopic>(ref<Letters>)) + option(SP) +
             some(append<&ast::CallNumber::subclasses>(ref<Digits>), lit<".">) +
             option(SP) + option(cuttersGroup) +
             option(SP +
                    ((assign<&ast::CallNumber::date>(ref<EditionRule>) +
                      option(SP + assign<&ast::CallNumber::item>(ref<ItemRule>))) |
                     assign<&ast::CallNumber::item>(ref<ItemRule>))) +
             option(SP)> {};

  using entry = CallNumberRule;
};

}  // namespace lcc
