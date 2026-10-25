#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "muparser/core/ast.hpp"

namespace muparser {

class MuparserValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkArity(const ast::FunctionCall&, cursiv::ValidationAcceptor&) const;
  void checkTarget(const ast::AssignExpr&, cursiv::ValidationAcceptor&) const;
};

struct MuparserCoreServices;
void register_validation_checks(MuparserCoreServices& services);

}  // namespace muparser
