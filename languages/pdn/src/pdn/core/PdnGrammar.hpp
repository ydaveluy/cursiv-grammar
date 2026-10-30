#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pdn/core/ast.hpp"

namespace pdn {

inline constexpr std::string_view interior(std::string_view text) {
  return text.substr(1, text.size() - 2);
}

struct Pdn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct COMMENT
      : terminal<as<interior>(lit<"{"> + many(cr<"^}">) + lit<"}">)> {};
  struct SETUP : terminal<as<interior>(lit<"/"> + many(cr<"^/">) + lit<"/">)> {
  };
  struct NAG : terminal<lit<"$"> + some(d)> {};
  struct STRENGTH
      : terminal<some(cr<"!?">) | (lit<"("> + some(cr<"!?">) + lit<")">)> {};
  struct NUMBER : terminal<some(d) + lit<"."> + option(lit<"..">)> {};
  struct SQUARE : terminal<some(d) | (cr<"a-h"> + cr<"1-8">)> {};
  struct ALPHAMOVE : terminal<cr<"a-h"> + cr<"1-8"> + cr<"a-h"> + cr<"1-8">> {};
  struct TEXT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};
  struct STRING
      : terminal<as<interior>(
            lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) + lit<"\"">)> {
  };

  struct WHITE_WIN : terminal<as<ast::Outcome::WhiteWin>(lit<"1-0"> + !d)> {};
  struct BLACK_WIN : terminal<as<ast::Outcome::BlackWin>(lit<"0-1"> + !d)> {};
  struct WHITE_WIN2 : terminal<as<ast::Outcome::WhiteWin2>(lit<"2-0"> + !d)> {};
  struct DRAW2 : terminal<as<ast::Outcome::Draw2>(lit<"1-1"> + !d)> {};
  struct BLACK_WIN2 : terminal<as<ast::Outcome::BlackWin2>(lit<"0-2"> + !d)> {};
  struct DOUBLE_FORFEIT
      : terminal<as<ast::Outcome::DoubleForfeit>(lit<"0-0"> + !d)> {};
  static constexpr auto outcome = as<ast::Outcome::Draw>(lit<"1/2-1/2">) | ref<WHITE_WIN> | ref<BLACK_WIN> |
                                  as<ast::Outcome::Unknown>(lit<"*">) | ref<WHITE_WIN2> | ref<DRAW2> |
                                  ref<BLACK_WIN2> | ref<DOUBLE_FORFEIT>;
  static constexpr auto outcome_marker = lit<"1/2-1/2"> | ref<WHITE_WIN> |
      ref<BLACK_WIN> | lit<"*"> | ref<WHITE_WIN2> | ref<DRAW2> |
      ref<BLACK_WIN2> | ref<DOUBLE_FORFEIT>;

  struct Tag : rule<lit<"["> + assign<&ast::Tag::name>(ref<TEXT>) +
                        assign<&ast::Tag::value>(ref<STRING>) + lit<"]">> {};
  struct Normal : rule<as<std::string_view>(!outcome_marker + ref<SQUARE> + lit<"-"> +
                                            ref<SQUARE>)> {};
  struct Capture
      : rule<as<std::string_view>(ref<SQUARE> + some(cr<"x:"> + ref<SQUARE>))> {
  };
  struct Ply : rule<option(assign<&ast::Ply::number>(ref<NUMBER>)) +
                        (flag<&ast::Ply::capture>(
                             assign<&ast::Ply::notation>(ref<Capture>)) |
                         assign<&ast::Ply::notation>(
                             ref<Normal> | ref<ALPHAMOVE> | lit<"...">)) +
                        option(assign<&ast::Ply::strength>(ref<STRENGTH>))> {};
  struct Comment : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};
  struct Setup : rule<assign<&ast::Setup::text>(ref<SETUP>)> {};
  struct Nag : rule<assign<&ast::Nag::value>(ref<NAG>)> {};
  struct Item;
  struct Variation
      : rule<lit<"("> + many(append<&ast::Variation::items>(ref<Item>)) +
             lit<")">> {};
  struct Item : rule<ref<Ply> | ref<Variation> | ref<Comment> |
                         ref<Setup> | ref<Nag>> {};
  struct Game : rule<create<ast::Game> +
                         ((some(append<&ast::Game::tags>(ref<Tag>)) +
                           many(append<&ast::Game::items>(ref<Item>))) |
                          some(append<&ast::Game::items>(ref<Item>))) +
                         many(append<&ast::Game::results>(outcome))> {};
  struct Document
      : rule<create<ast::Document> +
             many(append<&ast::Document::games>(ref<Game>))> {};

  using entry = Document;
  using ignored = WS;
  using hidden = LINE_COMMENT;
};

} // namespace pdn
