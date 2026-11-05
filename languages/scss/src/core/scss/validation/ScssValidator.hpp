// Context constraints are checked on the completed AST.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "scss/ast.hpp"

namespace scss {

class ScssValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkVariableFlags(const ast::VariableDeclaration&,
                          cursiv::ValidationAcceptor&) const;
  void checkReturnContext(const ast::ReturnStmt&,
                          cursiv::ValidationAcceptor&) const;
};

struct ScssCoreServices;
void register_validation_checks(ScssCoreServices& services);

}  // namespace scss
