#pragma once
#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "stl/core/ast.hpp"
#include <string_view>
namespace stl {
inline constexpr auto trim_name = [](std::string_view text) {
  const auto first = text.find_first_not_of(" \t\r\n\v\f");
  if (first == text.npos)
    return std::string_view{};
  return text.substr(first, text.find_last_not_of(" \t\r\n\v\f") - first + 1);
};
struct Stl : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<" \t\r\n\v\f">)> {};
  struct SOLID : terminal<cursiv::Lit<"solid", true>{}> {};
  struct ENDSOLID : terminal<cursiv::Lit<"endsolid", true>{}> {};
  struct FACET : terminal<cursiv::Lit<"facet", true>{}> {};
  struct NORMAL : terminal<cursiv::Lit<"normal", true>{}> {};
  struct OUTER : terminal<cursiv::Lit<"outer", true>{}> {};
  struct LOOP : terminal<cursiv::Lit<"loop", true>{}> {};
  struct ENDLOOP : terminal<cursiv::Lit<"endloop", true>{}> {};
  struct ENDFACET : terminal<cursiv::Lit<"endfacet", true>{}> {};
  struct VERTEX : terminal<cursiv::Lit<"vertex", true>{}> {};
  static constexpr auto infinity =
      cursiv::Lit<"inf", true>{} + option(cursiv::Lit<"inity", true>{});
  struct NUMBER : terminal<option(cr<"+-">) +
                           (some(cr<"0-9eE.+-">) |
                            (cursiv::Lit<"nan", true>{} +
                             option(lit<"("> + many(cr<"A-Za-z0-9_">) +
                                    lit<")">)) |
                            infinity)> {};
  struct EOL : terminal<lit<"\r\n"> | lit<"\n"> | lit<"\r">> {};
  inline static constexpr auto opening_name = [](std::string_view text) {
    return trim_name(text.substr(5));
  };
  inline static constexpr auto closing_name = [](std::string_view text) {
    return trim_name(text.substr(8));
  };
  struct SOLID_LINE
      : terminal<ref<SOLID> + many(cr<R"(^\r\n)">) + option(ref<EOL>),
                 text_conv<opening_name>> {};
  struct ENDSOLID_LINE
      : terminal<ref<ENDSOLID> + many(cr<R"(^\r\n)">) + option(ref<EOL>),
                 text_conv<closing_name>> {};
  struct TripleRule : rule<assign<&ast::Triple::i>(ref<NUMBER>) +
                           assign<&ast::Triple::j>(ref<NUMBER>) +
                           assign<&ast::Triple::k>(ref<NUMBER>)> {};
  struct VertexRule : rule<ref<VERTEX> + ref<TripleRule>> {};
  struct FacetRule : rule<ref<FACET> +
                          option(ref<NORMAL> +
                                 assign<&ast::Facet::normal>(ref<TripleRule>)) +
                          ref<OUTER> + ref<LOOP> +
                          many(append<&ast::Facet::vertices>(ref<VertexRule>)) +
                          ref<ENDLOOP> + ref<ENDFACET>> {};
  struct SolidRule
      : rule<assign<&ast::Solid::name>(ref<SOLID_LINE>) +
             many(append<&ast::Solid::facets>(ref<FacetRule>)) +
             option(flag<&ast::Solid::hasEnd>(
                 assign<&ast::Solid::endName>(ref<ENDSOLID_LINE>)))> {};
  struct DocRule : rule<create<ast::Document> +
                        many(append<&ast::Document::solids>(ref<SolidRule>))> {
  };
  using entry = DocRule;
  using ignore = WS;
};
} // namespace stl
