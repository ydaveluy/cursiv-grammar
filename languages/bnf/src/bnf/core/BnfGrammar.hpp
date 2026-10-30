#pragma once

#include <string_view>

#include "bnf/core/ast.hpp"
#include "cursiv/core/grammar/GrammarSpec.hpp"

namespace bnf {

inline constexpr auto trim = [](std::string_view t) {
  while (!t.empty() && (t.front() == ' ' || t.front() == '\t'))
    t.remove_prefix(1);
  while (!t.empty() && (t.back() == ' ' || t.back() == '\t'))
    t.remove_suffix(1);
  return t;
};

inline constexpr auto strip_assign = [](std::string_view t) {
  for (const std::string_view a :
       {std::string_view("::="), std::string_view(":≡"),
        std::string_view("-->"), std::string_view("→")})
    if (t.ends_with(a)) {
      t.remove_suffix(a.size());
      break;
    }
  return trim(t);
};

inline constexpr auto unbracket = [](std::string_view t) {
  t.remove_prefix(1);
  t.remove_suffix(1);
  return trim(t);
};


inline constexpr auto header_bracket_name = [](std::string_view t) {
  return unbracket(strip_assign(t));
};

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Bnf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto IdBody = cr<"a-zA-Z"> + some(cr<"a-zA-Z0-9_-">);
  struct ID : terminal<IdBody> {};

  static constexpr auto BracketBody = lit<"<"> + some(cr<R"(^>|\r\n)">) + lit<">">;
  struct BracketRef : terminal<as<unbracket>(BracketBody)> {};

  static constexpr auto AssignSpelling =
      lit<"::="> | lit<":≡"> | lit<"-->"> | lit<"→">;

  struct IdHeader
      : terminal<as<strip_assign>(cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">) +
                                  many(cr<R"( \t)">) + AssignSpelling)> {};
  struct BracketHeader : terminal<as<header_bracket_name>(
                             lit<"<"> + some(cr<R"(^>\r\n)">) + lit<">"> +
                             many(cr<R"( \t)">) + AssignSpelling)> {};

  struct QuotedText
      : terminal<as<unquote>((lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">) |
                             (lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">))> {};

  struct OR : terminal<lit<"|"> | lit<"o‾r‾"> | lit<"∣">> {};

  struct TEXT : terminal<!cr<R"( \t\r\n|)"> + !lit<"∣"> + any> {};

  struct Element
      : rule<flag<&ast::Element::nonterminal>(
                 assign<&ast::Element::text>(ref<ID> | ref<BracketRef>)) |
             assign<&ast::Element::text>(ref<QuotedText> | ref<TEXT>)> {};

  struct Alternative
      : rule<many(append<&ast::Alternative::elements>(ref<Element>))> {};

  struct Production
      : rule<assign<&ast::Production::name>(ref<IdHeader> |
                                            ref<BracketHeader>) +
             some(append<&ast::Production::alternatives>(ref<Alternative>),
                  ref<OR>)> {};

  struct Grammar
      : rule<create<ast::Grammar> +
             many(append<&ast::Grammar::productions>(ref<Production>))> {};

  using entry = Grammar;
  using ignored = WS;
};

}  // namespace bnf
