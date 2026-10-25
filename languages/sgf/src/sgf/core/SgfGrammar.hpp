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
  struct IDENTIFIER : terminal<some(cr<"A-Za-z">), text_conv<identifier>> {};

  inline static constexpr auto unbracket = [](std::string_view text) {
    return text.substr(1, text.size() - 2);
  };
  struct VALUE
      : terminal<lit<"["> + many((lit<"\\"> + any) | cr<R"(^\\])">) + lit<"]">,
                 text_conv<unbracket>> {};

  struct PropertyRule : rule<assign<&ast::Property::id>(ref<IDENTIFIER>) +
                             many(append<&ast::Property::values>(ref<VALUE>))> {
  };

  struct NodeRule
      : rule<lit<";"> +
             many(append<&ast::Node::properties>(ref<PropertyRule>))> {};

  struct GameTreeRule
      : rule<lit<"("> + some(append<&ast::GameTree::nodes>(ref<NodeRule>)) +
             many(append<&ast::GameTree::children>(ref<GameTreeRule>)) +
             lit<")">> {};

  struct CollectionRule
      : rule<some(append<&ast::Collection::trees>(ref<GameTreeRule>))> {};

  using entry = CollectionRule;
  using ignore = WS;
};

}  // namespace sgf
