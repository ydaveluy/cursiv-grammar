// Checks that do not require a database catalogue or executing the program.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "informix/core/ast.hpp"

namespace informix {

class InformixValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkLike(const ast::LikeExpr&, cursiv::ValidationAcceptor&) const;
  void checkMenuEvent(const ast::MenuEvent&, cursiv::ValidationAcceptor&) const;
  void checkMenu(const ast::MenuStmt&, cursiv::ValidationAcceptor&) const;
};

struct InformixCoreServices;
void register_validation_checks(InformixCoreServices& services);

}  // namespace informix
