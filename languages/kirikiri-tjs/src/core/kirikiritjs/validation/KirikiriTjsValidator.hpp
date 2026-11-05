// The checks of this language: what the grammar accepts but the language does
// not: handler uniqueness and trailing rest parameters.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "kirikiritjs/ast.hpp"

namespace kirikiritjs {

class KirikiriTjsValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFunction(const ast::FunctionDecl&, cursiv::ValidationAcceptor&) const;
  void checkFunctionExpr(const ast::FunctionExpr&, cursiv::ValidationAcceptor&) const;
  void checkProperty(const ast::PropertyDecl&, cursiv::ValidationAcceptor&) const;
};

struct KirikiriTjsCoreServices;
void register_validation_checks(KirikiriTjsCoreServices& services);

}  // namespace kirikiritjs
