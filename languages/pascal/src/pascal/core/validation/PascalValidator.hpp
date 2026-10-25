#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pascal/core/ast.hpp"

namespace pascal {

class PascalValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkAssignment(const ast::AssignStmt&,
                       cursiv::ValidationAcceptor&) const;
  void checkExpressionStatement(const ast::ExprStmt&,
                                cursiv::ValidationAcceptor&) const;
};

struct PascalCoreServices;
void register_validation_checks(PascalCoreServices& services);

}  // namespace pascal
