#pragma once

#include "cursiv/core/syntax-tree/AstNode.hpp"

namespace fen::ast {

struct PieceSquare : cursiv::Extends<cursiv::AstNode> {
  string_view piece;
  string_view promotion;
};
struct Rank : cursiv::Extends<cursiv::AstNode> {
  vector<variant<PieceSquare*, string_view>> squares;
};
struct Placement : cursiv::Extends<cursiv::AstNode> {
  vector<Rank*> ranks;
};
struct Fen : cursiv::Extends<cursiv::AstNode> {
  Placement* placement = nullptr;
  string_view color = "w";
  string_view castling = "-";
  string_view enpassant = "-";
  string_view halfmoveClock = "0";
  string_view fullmoveClock = "1";
  vector<string_view> extra;
};

}  // namespace fen::ast
