#pragma once

#include <memory_resource>
#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "sgf/core/ast.hpp"

namespace sgf {

struct Sgf : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \n\r\t\f\v)">)> {};

  inline static constexpr auto identifier = [](std::string_view text,
                                               std::pmr::memory_resource* mem) {
    if (text.find_first_of("abcdefghijklmnopqrstuvwxyz") == text.npos)
      return text;
    const auto room = cursiv::text_room(mem, text.size());
    std::size_t size = 0;
    for (char c : text)
      if (c >= 'A' && c <= 'Z') room[size++] = c;
    return std::string_view(room.data(), size);
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
