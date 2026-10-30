#pragma once

#include <string_view>

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "pgn/core/ast.hpp"

namespace pgn {
struct Pgn : cursiv::GrammarSpec {
  static constexpr auto word_boundary = cr<"a-zA-Z0-9_+#=:@-">;
  static constexpr auto Horizontal =
      cr<R"( \t\v\f\x1c-\x1f\u0085\u2028\u2029)"> | uc::Zs;
  // Percent escapes consume a physical line only after a line ending.
  // Treating inline '%' as a line comment silently discarded later moves.
  static constexpr auto PercentBody = lit<"%"> + many(cr<R"(^\r\n)">);
  struct WS : terminal<some(Horizontal | lit<"\xEF\xBB\xBF"> |
                            (eol + option(PercentBody)))> {};
  struct INITIAL_ESCAPE : terminal<PercentBody> {};
  struct REST_OF_LINE_COMMENT
      : terminal<lit<";"> + many(cr<R"(^\r\n)">)> {};
  struct BRACE_COMMENT
      : terminal<lit<"{"> + many(cr<"^}">) + option(lit<"}">)> {};
  inline static constexpr auto unquote = [](std::string_view text) {
    return text.substr(1, text.size() - 2);
  };
  struct STRING : terminal<as<unquote>(
                      lit<"\""> + many((lit<"\\"> + any) | cr<R"(^\\"\r\n)">) +
                      lit<"\"">)> {};
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
  struct NAG : terminal<as<nag_value>((lit<"$"> + some(d)) |
                                      (cr<"!?"> + option(cr<"!?">)))> {};
  static constexpr auto game_termination =
      as<ast::Result::white_wins>(kw<"1-0">) | as<ast::Result::black_wins>(kw<"0-1">) | as<ast::Result::drawn>(kw<"1/2-1/2">) | as<ast::Result::unknown>(lit<"*">);

  struct TagPair
      : rule<lit<"["> + assign<&ast::TagPair::name>(ref<SYMBOL>) +
             assign<&ast::TagPair::value>(ref<STRING>) + lit<"]">> {};
  struct MoveNumber
      : rule<(assign<&ast::MoveNumber::number>(ref<INTEGER>) +
              option(assign<&ast::MoveNumber::dots>(ref<DOTS>))) |
             assign<&ast::MoveNumber::dots>(ref<DOTS>)> {};
  struct SanMove : rule<assign<&ast::SanMove::san>(ref<SAN>)> {};
  struct Nag : rule<assign<&ast::Nag::number>(ref<NAG>)> {};
  struct Comment : rule<assign<&ast::Comment::raw>(ref<BRACE_COMMENT>)> {};
  struct UnexpectedClose : rule<create<ast::UnexpectedClose> + lit<")">> {};
  struct Variation;
  static constexpr auto element = ref<MoveNumber> | ref<SanMove> |
                                  ref<Nag> | ref<Comment> |
                                  ref<Variation>;
  struct Variation
      : rule<lit<"("> + many(append<&ast::Variation::elements>(element)) +
             (flag<&ast::Variation::closed>(lit<")">) | &game_termination |
              eof)> {};
  static constexpr auto BlankLine = gap::has(eol + many(Horizontal) + eol);
  static constexpr auto HeaderBreak =
      gap::has(eol + many(Horizontal) + eol + many(Horizontal) + eol);
  static constexpr auto terminated = flag<&ast::Game::terminated>(
      assign<&ast::Game::result>(game_termination));
  static constexpr auto top_element = element | ref<UnexpectedClose>;
  static constexpr auto body =
      (append<&ast::Game::elements>(top_element) +
       many(!BlankLine + append<&ast::Game::elements>(top_element)) +
       ((!BlankLine + terminated) | BlankLine | &lit<"["> | eof)) |
      terminated;
  struct Game
      : rule<create<ast::Game>(
            (append<&ast::Game::tags>(ref<TagPair>) +
             many(!HeaderBreak + append<&ast::Game::tags>(ref<TagPair>)) +
             ((!HeaderBreak + body) | HeaderBreak | eof)) |
            body)> {};
  struct Database
      : rule<create<ast::Database> + many(ref<INITIAL_ESCAPE>) +
             many(append<&ast::Database::games>(ref<Game>))> {};
  using entry = Database;
  using ignored = list<WS, REST_OF_LINE_COMMENT>;
};
}  // namespace pgn
