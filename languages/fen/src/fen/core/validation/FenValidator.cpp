#include "fen/core/validation/FenValidator.hpp"

#include <memory>
#include <string_view>
#include <variant>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "fen/core/CoreServices.hpp"
#include "fen/core/CounterValue.hpp"

namespace fen {
void FenValidator::checkFen(const ast::Fen& n,
                            cursiv::ValidationAcceptor& a) const {
  if (n.color != "w" && n.color != "b")
    a.error(n, "the active color must be w or b").property<&ast::Fen::color>();
  if (n.enpassant != "-" &&
      (n.enpassant.size() != 2 || n.enpassant[0] < 'a' ||
       n.enpassant[0] > 'h' || n.enpassant[1] < '1' || n.enpassant[1] > '8'))
    a.error(n, "the en passant target must be a square or -")
        .property<&ast::Fen::enpassant>();
  if (!counter_value(n.halfmoveClock))
    a.error(n, "the halfmove clock must be a nonnegative integer")
        .property<&ast::Fen::halfmoveClock>();
  if (!counter_value(n.fullmoveClock))
    a.error(n, "the fullmove number must be a nonnegative integer")
        .property<&ast::Fen::fullmoveClock>();
  if (!n.extra.empty())
    a.error(n, "FEN has at most six fields").property<&ast::Fen::extra>();
}
void FenValidator::checkPlacement(const ast::Placement& n,
                                  cursiv::ValidationAcceptor& a) const {
  if (n.ranks.size() != 8)
    a.error(n, "a board must contain eight ranks")
        .property<&ast::Placement::ranks>();
}
void FenValidator::checkRank(const ast::Rank& n,
                             cursiv::ValidationAcceptor& a) const {
  unsigned width = 0;
  for (const auto& square : n.squares) {
    if (std::holds_alternative<ast::PieceSquare*>(square))
      ++width;
    else {
      const auto run = std::get<std::string_view>(square);
      if (run.size() != 1 || run[0] < '1' || run[0] > '8') {
        a.error(n, "an empty-square run must be one digit from 1 to 8")
            .property<&ast::Rank::squares>();
        return;
      }
      width += run[0] - '0';
    }
  }
  if (width != 8)
    a.error(n, "a rank must contain eight squares")
        .property<&ast::Rank::squares>();
}
void FenValidator::checkPiece(const ast::PieceSquare& n,
                              cursiv::ValidationAcceptor& a) const {
  if (n.piece.size() != 1 ||
      std::string_view("prnbqkPRNBQK").find(n.piece[0]) ==
          std::string_view::npos)
    a.error(n, "unknown chess piece").property<&ast::PieceSquare::piece>();
  if (n.promotion.size() > 1)
    a.error(n, "a piece may have only one promotion marker")
        .property<&ast::PieceSquare::promotion>();
}
void FenValidator::checkCastling(const ast::Fen& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (n.castling == "-") return;
  unsigned white = 0, black = 0;
  bool lower = false;
  for (std::size_t i = 0; i < n.castling.size(); ++i) {
    const char c = n.castling[i];
    if (std::string_view("KQABCDEFGH").find(c) != std::string_view::npos &&
        !lower)
      ++white;
    else if (std::string_view("kqabcdefgh").find(c) != std::string_view::npos) {
      lower = true;
      ++black;
    } else {
      a.error(n, "invalid castling right or color order")
          .property<&ast::Fen::castling>();
      return;
    }
    if (n.castling.substr(0, i).find(c) != std::string_view::npos) {
      a.error(n, "a castling right is repeated")
          .property<&ast::Fen::castling>();
      return;
    }
  }
  if (white > 2 || black > 2 || white + black == 0)
    a.error(n, "each color has at most two castling rights")
        .property<&ast::Fen::castling>();
}
void register_validation_checks(FenCoreServices& services) {
  services.validator = std::make_unique<const FenValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&FenValidator::checkFen>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &FenValidator::checkPlacement>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &FenValidator::checkRank>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &FenValidator::checkPiece>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &FenValidator::checkCastling>(*services.validator)});
}
}  // namespace fen
