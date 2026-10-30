#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "icalendar/core/Parameters.hpp"
#include "icalendar/core/ast.hpp"

namespace icalendar {

struct Icalendar : cursiv::GrammarSpec {
  static constexpr auto newline = lit<"\r\n"> | lit<"\n"> | lit<"\r">;
  static constexpr auto fold = newline + cr<R"( \t)">;
  struct LINE_FOLD : terminal<fold> {};
  struct EOL : terminal<newline> {};
  static constexpr auto line_end = ref<EOL> | eof;

  struct NAME_RUN : terminal<some(cr<R"(^:;=\r\n \t)">)> {};
  struct HWS : terminal<some(cr<R"( \t)">)> {};
  struct Name : rule<as<property_name>(some(ref<NAME_RUN> | ref<HWS>))> {};
  struct ParamName : rule<as<trim_ows>(some(ref<NAME_RUN> | ref<HWS>))> {};
  struct VALUE_RUN : terminal<some(cr<R"(^\r\n)">)> {};
  struct Value : rule<many(ref<VALUE_RUN>)> {};
  struct Q_RUN : terminal<some(cr<R"(^"\r\n)">)> {};
  struct Quoted : rule<lit<"\""> + many(ref<Q_RUN>) + lit<"\"">> {};
  struct EscapedSeparator : rule<some(lit<"\\">) + cr<R"(,;:)">> {};
  struct PARAM_PIECE : terminal<cr<R"(^;:\r\n")">> {};
  struct ParamValue
      : rule<as<trim_ows>(
            many(ref<Quoted> | ref<EscapedSeparator> | ref<PARAM_PIECE>))> {};
  struct Param
      : rule<assign<&ast::Param::name>(ref<ParamName>) + lit<"="> +
             assign<&ast::Param::rawValue>(ref<ParamValue>)> {};

  static constexpr auto gap = many(cr<R"( \t)"> | fold);
  struct BEGIN
      : terminal<cr<"Bb"> + gap + cr<"Ee"> + gap + cr<"Gg"> + gap +
                 cr<"Ii"> + gap + cr<"Nn"> + gap + &cr<":;">> {};
  struct END
      : terminal<cr<"Ee"> + gap + cr<"Nn"> + gap + cr<"Dd"> + gap +
                 &cr<":;">> {};

  struct Property
      : rule<!(ref<BEGIN> | ref<END>) +
             assign<&ast::Property::name>(ref<Name>) +
             many(lit<";"> + append<&ast::Property::params>(ref<Param>)) +
             option(flag<&ast::Property::hasValueDelimiter>(lit<":">) + assign<&ast::Property::value>(ref<Value>)) +
             line_end> {};

  struct Component
      : rule<ref<BEGIN> +
             many(lit<";"> + append<&ast::Component::params>(ref<Param>)) + lit<":"> +
             assign<&ast::Component::name>(ref<Value>) + ref<EOL> +
             many(ref<EOL> |
                  append<&ast::Component::properties>(ref<Property>) |
                  append<&ast::Component::subcomponents>(ref<Component>)) +
             ref<END> +
             many(lit<";"> + append<&ast::Component::closeParams>(ref<Param>)) + lit<":"> +
             assign<&ast::Component::closeName>(ref<Value>) + line_end> {};

  struct Document
      : rule<create<ast::Document> +
             many(ref<EOL> | append<&ast::Document::components>(ref<Component>) |
                  append<&ast::Document::properties>(ref<Property>))> {};

  using entry = Document;
  using ignored = LINE_FOLD;
};

}  // namespace icalendar
