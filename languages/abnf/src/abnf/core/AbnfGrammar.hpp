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

  struct Element;
  struct Repetition
      : rule<option((option(assign<&ast::Repetition::min>(ref<INT>)) +
                     flag<&ast::Repetition::hasStar>(lit<"*">) +
                     option(assign<&ast::Repetition::max>(ref<INT>))) |
                    assign<&ast::Repetition::exact>(ref<INT>)) +
             assign<&ast::Repetition::element>(ref<Element>)> {};
  struct Concatenation
      : rule<some(append<&ast::Concatenation::repetitions>(ref<Repetition>))> {};
  struct Alternation
      : rule<some(append<&ast::Alternation::concatenations>(ref<Concatenation>),
                  lit<"/">)> {};
  struct Element
      : rule<assign<&ast::RuleRef::name>(ref<ID>) |
             (lit<"("> + assign<&ast::Group::alt>(ref<Alternation>) + lit<")">) |
             (lit<"["> + assign<&ast::OptionElem::alt>(ref<Alternation>) + lit<"]">) |
             assign<&ast::CharVal::text>(ref<STRING>) |
             assign<&ast::NumVal::text>(ref<NumberValue>) |
             assign<&ast::ProseVal::text>(ref<ProseValue>)> {};
  struct Rule
      : rule<assign<&ast::Rule::name>(ref<ID>) +
             (flag<&ast::Rule::incremental>(lit<"=/">) | lit<"=">) +
             assign<&ast::Rule::elements>(ref<Alternation>)> {};
  struct Rulelist : rule<many(append<&ast::Rulelist::rules>(ref<Rule>))> {};

  using entry = Rulelist;
  using ignored = WS;
  using hidden = COMMENT;
};

}  // namespace abnf
