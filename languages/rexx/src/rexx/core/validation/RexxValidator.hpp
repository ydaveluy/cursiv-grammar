// The checks of this language: none yet, nothing here needs a check beyond
// parsing.
#pragma once
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rexx/core/ast.hpp"

namespace rexx {

class RexxValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDoClauses(const ast::DoStmt&, cursiv::ValidationAcceptor&) const;
};

struct RexxCoreServices;
void register_validation_checks(RexxCoreServices& services);

}  // namespace rexx
