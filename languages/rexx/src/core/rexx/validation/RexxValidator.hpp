// Contextual checks on assignment symbols and controlled DO clauses.
#pragma once
#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "rexx/ast.hpp"

namespace rexx {

class RexxValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAssignment(const ast::Assignment&, cursiv::ValidationAcceptor&) const;
  void checkDoClauses(const ast::DoStmt&, cursiv::ValidationAcceptor&) const;
};

struct RexxCoreServices;
void register_validation_checks(RexxCoreServices& services);

}  // namespace rexx
