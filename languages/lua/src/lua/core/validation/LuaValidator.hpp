// The checks of this language: what the grammar accepts but the language
// does not. The grammar builds any suffixed expression as an assignment
// target or a bare-statement call (real Lua's own parser settles this with
// a runtime check, not a grammar rule); these two checks are that rule.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lua/core/ast.hpp"

namespace lua {

class LuaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkAssignTargets(const ast::AssignStat& stat,
                          cursiv::ValidationAcceptor& accept) const;
  void checkString(const ast::StringExpr&, cursiv::ValidationAcceptor&) const;
  void checkLocal(const ast::LocalStat&, cursiv::ValidationAcceptor&) const;
  void checkVararg(const ast::VarargExpr&, cursiv::ValidationAcceptor&) const;
  void checkBreak(const ast::BreakStat&, cursiv::ValidationAcceptor&) const;
  void checkCallStat(const ast::CallStat& stat,
                     cursiv::ValidationAcceptor& accept) const;
};

struct LuaCoreServices;
void register_validation_checks(LuaCoreServices& services);

}  // namespace lua
