#pragma once

#include <string>
#include <string_view>

#include "cursiv/grammar/GrammarSpec.hpp"
#include "sgf/ast.hpp"

namespace sgf {

struct Sgf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \n\r\t\f\v)">)> {};

  inline static constexpr auto identifier = [](std::string_view text) -> std::string {
    if (text.find_first_of("abcdefghijklmnopqrstuvwxyz") == text.npos)
      return std::string(text);
    std::string out;
    out.reserve(text.size());
    for (char c : text)
      if (c >= 'A' && c <= 'Z') out += c;
    return out;
  };
  struct IDENTIFIER : terminal<as<identifier>(some(cr<"A-Za-z">))> {};

  inline static constexpr auto unbracket = [](std::string_view text) {
    return text.substr(1, text.size() - 2);
  };
  struct VALUE
      : terminal<as<unbracket>(
            lit<"["> + many((lit<"\\"> + any) | cr<R"(^\\])">) + lit<"]">)> {};

  struct Property : rule<assign<&ast::Property::id>(ref<IDENTIFIER>) +
                             many(append<&ast::Property::values>(ref<VALUE>))> {
  };

  struct Node
      : rule<lit<";"> +
             many(append<&ast::Node::properties>(ref<Property>))> {};

  struct GameTree
      : rule<lit<"("> + some(append<&ast::GameTree::nodes>(ref<Node>)) +
             many(append<&ast::GameTree::children>(ref<GameTree>)) +
             lit<")">> {};

  struct Collection
      : rule<some(append<&ast::Collection::trees>(ref<GameTree>))> {};

  using entry = Collection;
  using ignored = WS;
};

}  // namespace sgf
