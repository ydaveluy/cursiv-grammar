#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pdn/core/ast.hpp"

namespace pdn {

inline constexpr std::string_view interior(std::string_view text) {
  return text.substr(1, text.size() - 2);
}
using Interior = cursiv::text_conv<interior>;

struct Pdn : cursiv::GrammarSpec {
  struct WS : terminal<some(cr<R"( \t\r\n)">)> {};
  struct LINE_COMMENT : terminal<lit<"%"> + many(cr<R"(^\r\n)">)> {};
  struct COMMENT : terminal<lit<"{"> + many(cr<"^}">) + lit<"}">, Interior> {};
  struct SETUP : terminal<lit<"/"> + many(cr<"^/">) + lit<"/">, Interior> {};
  struct NAG : terminal<lit<"$"> + some(d)> {};
  struct STRENGTH
      : terminal<some(cr<"!?">) | (lit<"("> + some(cr<"!?">) + lit<")">)> {};
  struct NUMBER : terminal<some(d) + lit<"."> + option(lit<"..">)> {};
  struct SQUARE : terminal<some(d) | (cr<"a-h"> + cr<"1-8">)> {};
  struct ALPHAMOVE : terminal<cr<"a-h"> + cr<"1-8"> + cr<"a-h"> + cr<"1-8">> {};
  struct TEXT : terminal<cr<"a-zA-Z"> + many(cr<"a-zA-Z0-9_">)> {};
  struct STRING : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^"\\)">) +
                               lit<"\"">,
                           Interior> {};

  struct DRAW : terminal<lit<"1/2-1/2">, value<ast::Outcome::Draw>> {};
  struct WHITE_WIN : terminal<lit<"1-0"> + !d, value<ast::Outcome::WhiteWin>> {
  };
  struct BLACK_WIN : terminal<lit<"0-1"> + !d, value<ast::Outcome::BlackWin>> {
  };
  struct UNKNOWN : terminal<lit<"*">, value<ast::Outcome::Unknown>> {};
  struct WHITE_WIN2
      : terminal<lit<"2-0"> + !d, value<ast::Outcome::WhiteWin2>> {};
  struct DRAW2 : terminal<lit<"1-1"> + !d, value<ast::Outcome::Draw2>> {};
  struct BLACK_WIN2
      : terminal<lit<"0-2"> + !d, value<ast::Outcome::BlackWin2>> {};
  struct DOUBLE_FORFEIT
      : terminal<lit<"0-0"> + !d, value<ast::Outcome::DoubleForfeit>> {};
  static constexpr auto outcome = ref<DRAW> | ref<WHITE_WIN> | ref<BLACK_WIN> |
                                  ref<UNKNOWN> | ref<WHITE_WIN2> | ref<DRAW2> |
                                  ref<BLACK_WIN2> | ref<DOUBLE_FORFEIT>;

  struct TagRule : rule<lit<"["> + assign<&ast::Tag::name>(ref<TEXT>) +
                        assign<&ast::Tag::value>(ref<STRING>) + lit<"]">> {};
  struct NormalRule : rule<!outcome + ref<SQUARE> + lit<"-"> + ref<SQUARE>,
                           std::string_view> {};
  struct CaptureRule
      : rule<ref<SQUARE> + some(cr<"x:"> + ref<SQUARE>), std::string_view> {};
  struct PlyRule : rule<option(assign<&ast::Ply::number>(ref<NUMBER>)) +
                        (flag<&ast::Ply::capture>(
                             assign<&ast::Ply::notation>(ref<CaptureRule>)) |
                         assign<&ast::Ply::notation>(
                             ref<NormalRule> | ref<ALPHAMOVE> | lit<"...">)) +
                        option(assign<&ast::Ply::strength>(ref<STRENGTH>))> {};
  struct CommentRule : rule<assign<&ast::Comment::text>(ref<COMMENT>)> {};
  struct SetupRule : rule<assign<&ast::Setup::text>(ref<SETUP>)> {};
  struct NagRule : rule<assign<&ast::Nag::value>(ref<NAG>)> {};
  struct ItemRule;
  struct VariationRule
      : rule<lit<"("> + many(append<&ast::Variation::items>(ref<ItemRule>)) +
             lit<")">> {};
  struct ItemRule : rule<ref<PlyRule> | ref<VariationRule> | ref<CommentRule> |
                         ref<SetupRule> | ref<NagRule>> {};
  struct GameRule : rule<create<ast::Game> +
                         ((some(append<&ast::Game::tags>(ref<TagRule>)) +
                           many(append<&ast::Game::items>(ref<ItemRule>))) |
                          some(append<&ast::Game::items>(ref<ItemRule>))) +
                         many(append<&ast::Game::results>(outcome))> {};
  struct DocumentRule
      : rule<create<ast::Document> +
             many(append<&ast::Document::games>(ref<GameRule>))> {};

  using entry = DocumentRule;
  using ignore = WS;
  using hidden = LINE_COMMENT;
};

} // namespace pdn
