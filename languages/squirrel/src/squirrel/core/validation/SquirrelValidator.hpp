// Checks on parameter lists shared by functions, lambdas and constructors.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "squirrel/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "squirrel/core/ast.hpp"

namespace squirrel {

class SquirrelValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFunction(const ast::FunctionDecl&, cursiv::ValidationAcceptor&) const;
  void checkFunctionExpr(const ast::FunctionExpr&, cursiv::ValidationAcceptor&) const;
  void checkLambda(const ast::LambdaExpr&, cursiv::ValidationAcceptor&) const;
  void checkConstructor(const ast::ConstructorDecl&, cursiv::ValidationAcceptor&) const;

};

struct SquirrelCoreServices;
void register_validation_checks(SquirrelCoreServices& services);

}  // namespace squirrel
