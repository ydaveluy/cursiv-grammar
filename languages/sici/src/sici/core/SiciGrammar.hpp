// The sici language: a grammar for the Serial Item and Contribution
// Identifier (ANSI/NISO Z39.56).
#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sici/core/ast.hpp"

namespace sici {

struct Sici : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  struct LETTER : terminal<cr<"a-zA-Z">> {};
  struct DIGIT : terminal<d> {};
  struct ENUM_PART : terminal<some(cr<R"(^:<>+* \t\r\n)">)> {};
  struct FIELD : terminal<some(cr<R"(^:<> \t\r\n)">)> {};
  struct LOCAL_NUMBER : terminal<some(cr<R"(^<> \t\r\n)">)> {};
  struct VERSION : terminal<some(d)> {};
  struct CHECK : terminal<cr<"0-9A-Za-z#">> {};

  struct Issn : rule<ref<DIGIT> + ref<DIGIT> + ref<DIGIT> + ref<DIGIT> +
                     lit<"-"> + ref<DIGIT> + ref<DIGIT> + ref<DIGIT> +
                     (ref<DIGIT> | lit<"X"> | lit<"x">)> {};
  struct ChronologyRun : rule<some(ref<DIGIT> | lit<"/">)> {};
  struct Mfi : rule<repeat<2>(ref<LETTER>)> {};

  struct EnumerationRule
      : rule<(assign<&ast::Enumeration::volume>(ref<ENUM_PART>) +
                 option(lit<":"> +
                        assign<&ast::Enumeration::issue>(ref<ENUM_PART>)) +
                 many(lit<":"> +
                      append<&ast::Enumeration::additional>(ref<ENUM_PART>)) +
                 option(option(lit<":">) +
                        assign<&ast::Enumeration::supplement>(cr<"+*">))) |
             assign<&ast::Enumeration::supplement>(cr<"+*">)> {};

  struct ItemRule
      : rule<assign<&ast::Item::issn>(ref<Issn>) + lit<"("> +
             option(assign<&ast::Item::chronology>(ref<ChronologyRun>)) +
             lit<")"> +
             option(assign<&ast::Item::enumeration>(ref<EnumerationRule>))> {};

  struct ContributionRule
      : rule<create<ast::Contribution> + lit<"<"> +
             option(assign<&ast::Contribution::location>(ref<FIELD>)) +
             option(lit<":"> +
                    option(assign<&ast::Contribution::title>(ref<FIELD>)) +
                    option(lit<":"> +
                           option(assign<&ast::Contribution::localNumber>(
                               ref<LOCAL_NUMBER>)))) +
             lit<">">> {};

  struct ControlRule
      : rule<assign<&ast::Control::csi>(ref<DIGIT>) + lit<"."> +
             assign<&ast::Control::dpi>(ref<DIGIT>) + lit<"."> +
             assign<&ast::Control::mfi>(ref<Mfi>) + lit<";"> +
             assign<&ast::Control::version>(ref<VERSION>) + option(lit<"-">) +
             assign<&ast::Control::check>(ref<CHECK>)> {};

  struct SiciRule
      : rule<assign<&ast::Sici::item>(ref<ItemRule>) +
             assign<&ast::Sici::contribution>(ref<ContributionRule>) +
             many(append<&ast::Sici::controls>(ref<ControlRule>))> {};

  using entry = SiciRule;
  using ignore = WS;
};

}  // namespace sici
