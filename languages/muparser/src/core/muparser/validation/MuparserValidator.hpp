#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "muparser/ast.hpp"

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
