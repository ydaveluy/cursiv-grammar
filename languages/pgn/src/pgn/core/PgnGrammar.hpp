#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pgn/core/ast.hpp"

namespace pgn {
struct Pgn : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_+#=:@-">;
  static constexpr auto Horizontal =
      cr<R"( \t\v\f\x1c-\x1f\u0085\u2028\u2029)"> | uc::Zs;
  struct WS : terminal<some(Horizontal | cr<R"(\r\n\uFEFF)">)> {};
  struct REST_OF_LINE_COMMENT
      : terminal<(lit<";"> | lit<"%">)+many(cr<R"(^\r\n)">)> {};
  struct BRACE_COMMENT
      : terminal<lit<"{"> + many(cr<"^}">) + option(lit<"}">)> {};
  inline static constexpr auto unquote = [](std::string_view text) {
    return text.substr(1, text.size() - 2);
  };
  struct STRING
      : terminal<lit<"\""> + many((lit<"\\"> + any) | cr<R"(^\\"\r\n)">) +
                     lit<"\"">,
                 text_conv<unquote>> {};
  struct INTEGER : terminal<!(lit<"0000"> + !d) + some(d)> {};
  struct DOTS : rule<some(lit<".">)> {};
  struct SYMBOL : terminal<cr<"a-zA-Z0-9"> + many(cr<"a-zA-Z0-9_+#=:-">)> {};
  struct SAN : terminal<lit<"0000"> | lit<"--"> | lit<"@@@@"> |
                        (cr<"a-zA-Z0-9"> + many(cr<"a-zA-Z0-9_+#=:@-">))> {};
  inline static constexpr auto nag_value =
      [](std::string_view text) -> std::string_view {
    if (text.starts_with('$')) return text.substr(1);
    if (text == "!") return "1";
    if (text == "?") return "2";
    if (text == "!!") return "3";
    if (text == "??") return "4";
    if (text == "!?") return "5";
    return "6";
  };
  struct NAG : terminal<(lit<"$"> + some(d)) | (cr<"!?"> + option(cr<"!?">)),
                        text_conv<nag_value>> {};
  struct WHITE_WINS : terminal<kw<"1-0">, value<ast::Result::white_wins>> {};
  struct BLACK_WINS : terminal<kw<"0-1">, value<ast::Result::black_wins>> {};
  struct DRAWN_GAME : terminal<kw<"1/2-1/2">, value<ast::Result::drawn>> {};
  struct ASTERISK : terminal<lit<"*">, value<ast::Result::unknown>> {};
  static constexpr auto game_termination =
      ref<WHITE_WINS> | ref<BLACK_WINS> | ref<DRAWN_GAME> | ref<ASTERISK>;

  struct TagPairRule
      : rule<lit<"["> + assign<&ast::TagPair::name>(ref<SYMBOL>) +
             assign<&ast::TagPair::value>(ref<STRING>) + lit<"]">> {};
  struct MoveNumberRule
      : rule<(assign<&ast::MoveNumber::number>(ref<INTEGER>) +
              option(assign<&ast::MoveNumber::dots>(ref<DOTS>))) |
             assign<&ast::MoveNumber::dots>(ref<DOTS>)> {};
  struct SanMoveRule : rule<assign<&ast::SanMove::san>(ref<SAN>)> {};
  struct NagRule : rule<assign<&ast::Nag::number>(ref<NAG>)> {};
  struct CommentRule : rule<assign<&ast::Comment::raw>(ref<BRACE_COMMENT>)> {};
  struct UnexpectedCloseRule : rule<create<ast::UnexpectedClose> + lit<")">> {};
  struct VariationRule;
  static constexpr auto element = ref<MoveNumberRule> | ref<SanMoveRule> |
                                  ref<NagRule> | ref<CommentRule> |
                                  ref<VariationRule>;
  struct VariationRule
      : rule<lit<"("> + many(append<&ast::Variation::elements>(element)) +
             (flag<&ast::Variation::closed>(lit<")">) | &game_termination |
              eof)> {};
  static constexpr auto BlankLine = gap::has(eol + many(Horizontal) + eol);
  static constexpr auto HeaderBreak =
      gap::has(eol + many(Horizontal) + eol + many(Horizontal) + eol);
  static constexpr auto terminated = flag<&ast::Game::terminated>(
      assign<&ast::Game::result>(game_termination));
  static constexpr auto top_element = element | ref<UnexpectedCloseRule>;
  static constexpr auto body =
      (append<&ast::Game::elements>(top_element) +
       many(!BlankLine + append<&ast::Game::elements>(top_element)) +
       ((!BlankLine + terminated) | BlankLine | &lit<"["> | eof)) |
      terminated;
  struct GameRule
      : rule<create<ast::Game>(
            (append<&ast::Game::tags>(ref<TagPairRule>) +
             many(!HeaderBreak + append<&ast::Game::tags>(ref<TagPairRule>)) +
             ((!HeaderBreak + body) | HeaderBreak | eof)) |
            body)> {};
  struct DatabaseRule
      : rule<create<ast::Database> +
             many(append<&ast::Database::games>(ref<GameRule>))> {};
  using entry = DatabaseRule;
  using ignore = list<WS, REST_OF_LINE_COMMENT>;
};
}  // namespace pgn
