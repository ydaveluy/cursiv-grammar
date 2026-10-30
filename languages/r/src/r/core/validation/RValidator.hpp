// The checks of this language: what the grammar accepts but the language
// does not: source-context restrictions and literal/parameter checks.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "r/core/ast.hpp"

namespace r {

class RValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFunction(const ast::FunctionDef&, cursiv::ValidationAcceptor&) const;
  void checkBinary(const ast::BinaryExpr&, cursiv::ValidationAcceptor&) const;
  void checkPlaceholder(const ast::Placeholder&, cursiv::ValidationAcceptor&) const;
  void checkAssignment(const ast::AssignExpr&, cursiv::ValidationAcceptor&) const;
  void checkIf(const ast::IfExpr&, cursiv::ValidationAcceptor&) const;
};

struct RCoreServices;
void register_validation_checks(RCoreServices& services);

}  // namespace r
