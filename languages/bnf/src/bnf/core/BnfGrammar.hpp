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
using Unbracketed = cursiv::text_conv<unbracket>;

using HeaderName = cursiv::text_conv<strip_assign>;

inline constexpr auto header_bracket_name = [](std::string_view t) {
  return unbracket(strip_assign(t));
};
using HeaderBracketName = cursiv::text_conv<header_bracket_name>;

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};
using Unquoted = cursiv::text_conv<unquote>;

struct Bnf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto IdBody = cr<"a-zA-Z"> + some(cr<"a-zA-Z0-9_-">);
  struct ID : terminal<IdBody> {};

  static constexpr auto BracketBody = lit<"<"> + some(cr<R"(^>|\r\n)">) + lit<">">;
  struct BracketRef : terminal<BracketBody, Unbracketed> {};

  static constexpr auto AssignSpelling =
      lit<"::="> | lit<":≡"> | lit<"-->"> | lit<"→">;

  struct IdHeader
      : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_-">) +
                 many(cr<R"( \t)">) + AssignSpelling, HeaderName> {};
  struct BracketHeader
      : terminal<lit<"<"> + some(cr<R"(^>\r\n)">) + lit<">"> +
                 many(cr<R"( \t)">) + AssignSpelling,
                 HeaderBracketName> {};

  struct QuotedText : terminal<(lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">) |
                                   (lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">),
                               Unquoted> {};

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

  struct GrammarRule
      : rule<create<ast::Grammar> +
             many(append<&ast::Grammar::productions>(ref<Production>))> {};

  using entry = GrammarRule;
  using ignore = WS;
};

}  // namespace bnf
