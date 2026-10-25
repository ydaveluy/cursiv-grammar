#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "icalendar/core/Parameters.hpp"
#include "icalendar/core/ast.hpp"

namespace icalendar {

struct Icalendar : cursiv::GrammarSpec {
  static constexpr auto newline = lit<"\r\n"> | lit<"\n"> | lit<"\r">;
  static constexpr auto fold = some(newline) + cr<R"( \t)">;
  struct LINE_FOLD : terminal<fold> {};
  struct EOL : terminal<newline> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct NAME_RUN : terminal<some(cr<R"(^:;=\r\n \t)">)> {};
  struct HWS : terminal<some(cr<R"( \t)">)> {};
  struct Name : rule<some(ref<NAME_RUN> | ref<HWS>),
                     text_conv<property_name>> {};
  struct ParamName : rule<some(ref<NAME_RUN> | ref<HWS>),
                          text_conv<trim_ows>> {};
  struct VALUE_RUN : terminal<some(cr<R"(^\r\n)">)> {};
  struct Value : rule<many(ref<VALUE_RUN>)> {};
  struct Q_RUN : terminal<some(cr<R"(^"\r\n)">)> {};
  struct Quoted : rule<lit<"\""> + many(ref<Q_RUN>) + lit<"\"">> {};
  struct EscapedSeparator : rule<some(lit<"\\">) + cr<R"(,;:)">> {};
  struct PARAM_PIECE : terminal<cr<R"(^;:\r\n")">> {};
  struct ParamValue : rule<many(ref<Quoted> | ref<EscapedSeparator> | ref<PARAM_PIECE>),
                           text_conv<trim_ows>> {};
  struct ParamRule
      : rule<assign<&ast::Param::name>(ref<ParamName>) + lit<"="> +
             assign<&ast::Param::rawValue>(ref<ParamValue>)> {};

  static constexpr auto gap = many(cr<R"( \t)"> | fold);
  struct BEGIN
      : terminal<cr<"Bb"> + gap + cr<"Ee"> + gap + cr<"Gg"> + gap +
                 cr<"Ii"> + gap + cr<"Nn"> + gap + &cr<":;">> {};
  struct END
      : terminal<cr<"Ee"> + gap + cr<"Nn"> + gap + cr<"Dd"> + gap +
                 &cr<":;">> {};

  struct PropertyRule
      : rule<!(ref<BEGIN> | ref<END>) +
             assign<&ast::Property::name>(ref<Name>) +
             many(lit<";"> + append<&ast::Property::params>(ref<ParamRule>)) +
             option(flag<&ast::Property::hasValueDelimiter>(lit<":">) + assign<&ast::Property::value>(ref<Value>)) +
             line_end> {};

  struct ComponentRule
      : rule<ref<BEGIN> +
             many(lit<";"> + append<&ast::Component::params>(ref<ParamRule>)) + lit<":"> +
             assign<&ast::Component::name>(ref<Value>) + line_end +
             many(ref<EOL> |
                  append<&ast::Component::properties>(ref<PropertyRule>) |
                  append<&ast::Component::subcomponents>(ref<ComponentRule>)) +
             ref<END> +
             many(lit<";"> + append<&ast::Component::closeParams>(ref<ParamRule>)) + lit<":"> +
             assign<&ast::Component::closeName>(ref<Value>) + line_end> {};

  struct DocumentRule
      : rule<create<ast::Document> +
             many(ref<EOL> | append<&ast::Document::components>(ref<ComponentRule>) |
                  append<&ast::Document::properties>(ref<PropertyRule>))> {};

  using entry = DocumentRule;
  using ignore = LINE_FOLD;
};

}  // namespace icalendar
