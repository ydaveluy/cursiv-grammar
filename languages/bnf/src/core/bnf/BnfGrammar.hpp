#pragma once

#include <string_view>

#include "bnf/ast.hpp"
#include "cursiv/grammar/GrammarSpec.hpp"

namespace bnf {

inline constexpr auto trim = [](std::string_view t) {
  while (!t.empty() && (t.front() == ' ' || t.front() == '\t'))
    t.remove_prefix(1);
  while (!t.empty() && (t.back() == ' ' || t.back() == '\t'))
    t.remove_suffix(1);
  return t;
};

inline constexpr auto unbracket = [](std::string_view t) {
  t.remove_prefix(1);
  t.remove_suffix(1);
  return trim(t);
};

inline constexpr auto unquote = [](std::string_view t) {
  return t.substr(1, t.size() - 2);
};

struct Bnf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};

  static constexpr auto Letter = cr<"a-zA-Z">;
  static constexpr auto NameChar = !lit<"-->"> + cr<"a-zA-Z0-9_-">;
  struct ID : terminal<Letter + some(NameChar)> {};
  struct Assign : terminal<lit<"::="> | lit<":≡"> | lit<"-->"> | lit<"→">> {};
  // A header may contain bars. On the right they separate alternatives.
  struct BracketRef : terminal<as<unbracket>(
      lit<"<"> + ((some(cr<R"(^>|\r\n)">) + lit<">">) |
                  (some(cr<R"(^>\r\n)">) + lit<">"> + &(many(cr<R"( \t)">) + ref<Assign>))))> {};

  struct QuotedText
      : terminal<as<unquote>((lit<"\""> + many(cr<R"(^"\r\n)">) + lit<"\"">) |
                             (lit<"'"> + many(cr<R"(^'\r\n)">) + lit<"'">))> {};

  struct OR : terminal<lit<"|"> | lit<"o‾r‾"> | lit<"∣">> {};

  struct TEXT : terminal<!cr<R"( \t\r\n|)"> + !lit<"∣"> + any> {};

  static constexpr auto Name = ref<ID> | Letter | ref<BracketRef>;
  static constexpr auto Assignment = !gap::line + ref<Assign>;
  struct Element
      : rule<((!gap::line) | !(Name + Assignment)) +
             (flag<&ast::Element::nonterminal>(
                  assign<&ast::Element::text>(ref<ID> | ref<BracketRef>)) |
              assign<&ast::Element::text>(ref<QuotedText> | ref<Assign> | ref<TEXT>))> {};

  struct Alternative
      : rule<many(append<&ast::Alternative::elements>(ref<Element>))> {};

  struct Production
      : rule<assign<&ast::Production::name>(Name) + Assignment +
             some(append<&ast::Production::alternatives>(ref<Alternative>),
                  ref<OR>)> {};

  struct Grammar
      : rule<create<ast::Grammar> +
             many(append<&ast::Grammar::productions>(ref<Production>), gap::line)> {};

  using entry = Grammar;
  using ignored = WS;
};

}  // namespace bnf
