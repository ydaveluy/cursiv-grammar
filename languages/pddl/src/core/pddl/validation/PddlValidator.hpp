// Contextual checks over the generated syntax tree.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pddl/ast.hpp"

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
