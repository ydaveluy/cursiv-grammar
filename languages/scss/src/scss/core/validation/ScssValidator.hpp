// Context constraints are checked on the completed AST.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "scss/core/ast.hpp"

namespace scss {

class ScssValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkReturnContext(const ast::ReturnStmt&,
                          cursiv::ValidationAcceptor&) const;
};

struct ScssCoreServices;
void register_validation_checks(ScssCoreServices& services);

}  // namespace scss
