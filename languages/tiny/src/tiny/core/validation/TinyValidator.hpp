#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tiny/core/ast.hpp"

namespace tiny {

class TinyValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkVarRef(const ast::VarRef& node, cursiv::ValidationAcceptor& accept) const;
  void checkAssign(const ast::AssignStmt& node, cursiv::ValidationAcceptor& accept) const;
  void checkRead(const ast::ReadStmt& node, cursiv::ValidationAcceptor& accept) const;
};

struct TinyCoreServices;
void register_validation_checks(TinyCoreServices& services);

}  // namespace tiny
