#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pgn/core/ast.hpp"
namespace pgn {
class PgnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGame(const ast::Game&, cursiv::ValidationAcceptor&) const;
  void checkVariation(const ast::Variation&, cursiv::ValidationAcceptor&) const;
  void checkComment(const ast::Comment&, cursiv::ValidationAcceptor&) const;
  void checkUnexpectedClose(const ast::UnexpectedClose&,
                            cursiv::ValidationAcceptor&) const;
};
struct PgnCoreServices;
void register_validation_checks(PgnCoreServices& services);
}  // namespace pgn
