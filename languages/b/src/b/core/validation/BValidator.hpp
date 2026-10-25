#pragma once
#include "b/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace b {
class BValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkAssignment(const ast::AssignExpr &,
                       cursiv::ValidationAcceptor &) const;
  void checkPrefix(const ast::PrefixExpr &, cursiv::ValidationAcceptor &) const;
  void checkPostfix(const ast::PostfixExpr &,
                    cursiv::ValidationAcceptor &) const;
  void checkAddress(const ast::AddrExpr &, cursiv::ValidationAcceptor &) const;
  void checkVariable(const ast::VarDef &, cursiv::ValidationAcceptor &) const;
  void checkAuto(const ast::AutoVar &, cursiv::ValidationAcceptor &) const;
  void checkIdentifier(const ast::Ident &, cursiv::ValidationAcceptor &) const;
  void checkCase(const ast::CaseLabel &, cursiv::ValidationAcceptor &) const;
};
struct BCoreServices;
void register_validation_checks(BCoreServices &);
} // namespace b
