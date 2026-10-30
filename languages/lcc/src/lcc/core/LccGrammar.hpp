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

  struct Cutter
      : rule<assign<&ast::Cutter::letter>(ref<Letters>) + option(SP) +
             assign<&ast::Cutter::digits>(ref<Digits>)> {};

  static constexpr auto anotherCutter = &ref<CutterLetters> + ref<Cutter>;
  static constexpr auto cuttersGroup = option(lit<".">) + option(SP) +
      append<&ast::CallNumber::cutters>(ref<Cutter>) +
      many(option(SP) + append<&ast::CallNumber::cutters>(anotherCutter));

  struct Edition : rule<as<std::string_view>(&ref<Year> + ref<Digits> +
                                             option(ref<Letters>))> {};
  struct Item
      : rule<as<std::string_view>(some(ref<Letters> | ref<Digits> | lit<"."> |
                                       ref<Punctuation> | SP))> {};

  struct CallNumber
      : rule<option(SP) + assign<&ast::CallNumber::topic>(ref<Letter>) +
             option(assign<&ast::CallNumber::subtopic>(ref<Letters>)) + option(SP) +
             some(append<&ast::CallNumber::subclasses>(ref<Digits>), lit<".">) +
             option(SP) + option(cuttersGroup) +
             option(SP +
                    ((assign<&ast::CallNumber::date>(ref<Edition>) +
                      option(SP + assign<&ast::CallNumber::item>(ref<Item>))) |
                     assign<&ast::CallNumber::item>(ref<Item>))) +
             option(SP)> {};

  using entry = CallNumber;
};

}  // namespace lcc
