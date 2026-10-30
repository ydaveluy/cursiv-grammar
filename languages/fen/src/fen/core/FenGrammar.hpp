#pragma once

#include "cursiv/core/grammar/GrammarSpec.hpp"
#include "fen/core/ast.hpp"

namespace fen {

struct Fen : cursiv::GrammarSpec {
  static constexpr auto Whitespace =
      cr<R"(\t\n\v\f\r\x1c-\x20\u0085\u2028\u2029)"> | uc::Zs;
  struct WS : terminal<some(Whitespace)> {};
  struct FIELD : terminal<some(!Whitespace + any)> {};
  struct NUMBER : terminal<some(cr<"0-9">)> {};
  struct PIECE : terminal<cr<"A-Za-z">> {};
  struct PROMOTION : terminal<some(lit<"~">)> {};

  struct Piece
      : rule<assign<&ast::PieceSquare::piece>(ref<PIECE>) +
             option(assign<&ast::PieceSquare::promotion>(ref<PROMOTION>))> {};
  struct Rank : rule<some(append<&ast::Rank::squares>(ref<Piece>) |
                              append<&ast::Rank::squares>(ref<NUMBER>))> {};
  struct Placement
      : rule<some(append<&ast::Placement::ranks>(ref<Rank>), lit<"/">)> {};
  struct FenRule
      : rule<create<ast::Fen>(
            option(ref<WS>) + assign<&ast::Fen::placement>(ref<Placement>) +
            option(
                ref<WS> + assign<&ast::Fen::color>(ref<FIELD>) +
                option(
                    ref<WS> + assign<&ast::Fen::castling>(ref<FIELD>) +
                    option(
                        ref<WS> + assign<&ast::Fen::enpassant>(ref<FIELD>) +
                        option(ref<WS> +
                               assign<&ast::Fen::halfmoveClock>(ref<FIELD>) +
                               option(ref<WS> +
                                      assign<&ast::Fen::fullmoveClock>(
                                          ref<FIELD>) +
                                      many(ref<WS> + append<&ast::Fen::extra>(
                                                         ref<FIELD>))))))) +
            option(ref<WS>) + eof)> {};
  using entry = FenRule;
};

}  // namespace fen
