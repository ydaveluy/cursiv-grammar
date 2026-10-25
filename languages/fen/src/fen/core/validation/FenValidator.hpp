#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "fen/core/ast.hpp"

namespace fen {
class FenValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFen(const ast::Fen&, cursiv::ValidationAcceptor&) const;
  void checkPlacement(const ast::Placement&, cursiv::ValidationAcceptor&) const;
  void checkRank(const ast::Rank&, cursiv::ValidationAcceptor&) const;
  void checkPiece(const ast::PieceSquare&, cursiv::ValidationAcceptor&) const;
  void checkCastling(const ast::Fen&, cursiv::ValidationAcceptor&) const;
};
struct FenCoreServices;
void register_validation_checks(FenCoreServices& services);
}  // namespace fen
