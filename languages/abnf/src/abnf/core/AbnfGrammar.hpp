#pragma once

#include "abnf/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace abnf {

struct Abnf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct COMMENT : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  struct ID : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">)> {};
  struct INT : terminal<some(d)> {};
  struct STRING
      : terminal<option(lit<"%"> + cr<"sSiI">) + lit<"\""> +
                 many(cr<R"(^")">) + lit<"\"">> {};
  struct NumberValue
      : terminal<lit<"%"> + cr<"bBdDxX"> + some(cr<"0-9a-fA-F">) +
                 many(cr<".-"> + some(cr<"0-9a-fA-F">))> {};
  struct ProseValue : terminal<lit<"<"> + many(cr<"^>">) + lit<">">> {};

  struct ElementRule;
  struct RepetitionRule
      : rule<option((option(assign<&ast::Repetition::min>(ref<INT>)) +
                     flag<&ast::Repetition::hasStar>(lit<"*">) +
                     option(assign<&ast::Repetition::max>(ref<INT>))) |
                    assign<&ast::Repetition::exact>(ref<INT>)) +
             assign<&ast::Repetition::element>(ref<ElementRule>)> {};
  struct ConcatenationRule
      : rule<some(append<&ast::Concatenation::repetitions>(ref<RepetitionRule>))> {};
  struct AlternationRule
      : rule<some(append<&ast::Alternation::concatenations>(ref<ConcatenationRule>),
                  lit<"/">)> {};
  struct ElementRule
      : rule<assign<&ast::RuleRef::name>(ref<ID>) |
             (lit<"("> + assign<&ast::Group::alt>(ref<AlternationRule>) + lit<")">) |
             (lit<"["> + assign<&ast::OptionElem::alt>(ref<AlternationRule>) + lit<"]">) |
             assign<&ast::CharVal::text>(ref<STRING>) |
             assign<&ast::NumVal::text>(ref<NumberValue>) |
             assign<&ast::ProseVal::text>(ref<ProseValue>)> {};
  struct RuleRule
      : rule<assign<&ast::Rule::name>(ref<ID>) +
             (flag<&ast::Rule::incremental>(lit<"=/">) | lit<"=">) +
             assign<&ast::Rule::elements>(ref<AlternationRule>)> {};
  struct RulelistRule : rule<many(append<&ast::Rulelist::rules>(ref<RuleRule>))> {};

  using entry = RulelistRule;
  using ignore = WS;
  using hidden = COMMENT;
};

}  // namespace abnf
