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
  struct EOL : terminal<eol> {};
  inline static constexpr auto opening_name = [](std::string_view text) {
    return trim_name(text.substr(5));
  };
  inline static constexpr auto closing_name = [](std::string_view text) {
    return trim_name(text.substr(8));
  };
  struct SOLID_LINE
      : terminal<as<opening_name>(ref<SOLID> + many(cr<R"(^\r\n)">) +
                                  option(ref<EOL>))> {};
  struct ENDSOLID_LINE
      : terminal<as<closing_name>(ref<ENDSOLID> + many(cr<R"(^\r\n)">) +
                                  option(ref<EOL>))> {};
  struct Triple : rule<assign<&ast::Triple::i>(ref<NUMBER>) +
                           assign<&ast::Triple::j>(ref<NUMBER>) +
                           assign<&ast::Triple::k>(ref<NUMBER>)> {};
  struct Vertex : rule<ref<VERTEX> + ref<Triple>> {};
  struct Facet : rule<ref<FACET> +
                          option(ref<NORMAL> +
                                 assign<&ast::Facet::normal>(ref<Triple>)) +
                          ref<OUTER> + ref<LOOP> +
                          many(append<&ast::Facet::vertices>(ref<Vertex>)) +
                          ref<ENDLOOP> + ref<ENDFACET>> {};
  struct Solid
      : rule<assign<&ast::Solid::name>(ref<SOLID_LINE>) +
             many(append<&ast::Solid::facets>(ref<Facet>)) +
             option(flag<&ast::Solid::hasEnd>(
                 assign<&ast::Solid::endName>(ref<ENDSOLID_LINE>)))> {};
  struct Doc : rule<create<ast::Document> +
                        many(append<&ast::Document::solids>(ref<Solid>))> {
  };
  using entry = Doc;
  using ignored = WS;
};
} // namespace stl
