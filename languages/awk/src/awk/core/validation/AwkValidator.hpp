// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "awk/core/ast.hpp"

namespace awk {

class AwkValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueFunctionNames(const ast::Program& program,
                                cursiv::ValidationAcceptor& accept) const;
  void checkAssignment(const ast::AssignExpr& expression,
                       cursiv::ValidationAcceptor& accept) const;
  void checkBuiltin(const ast::CallExpr& call,
                    cursiv::ValidationAcceptor& accept) const;
  void checkRule(const ast::Rule& rule,
                 cursiv::ValidationAcceptor& accept) const;
  void checkDelete(const ast::DeleteStmt& statement,
                   cursiv::ValidationAcceptor& accept) const;
  void checkSwitch(const ast::SwitchStmt& statement,
                   cursiv::ValidationAcceptor& accept) const;
};

struct AwkCoreServices;
void register_validation_checks(AwkCoreServices& services);

}  // namespace awk
