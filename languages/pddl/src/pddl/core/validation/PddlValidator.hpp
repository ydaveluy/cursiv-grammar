// Contextual checks over the generated syntax tree.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pddl/core/ast.hpp"

namespace pddl {

class PddlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkExpression(const ast::Expr&, cursiv::ValidationAcceptor&) const;
  void checkEffect(const ast::Effect&, cursiv::ValidationAcceptor&) const;
};

struct PddlCoreServices;
void register_validation_checks(PddlCoreServices& services);

}  // namespace pddl
