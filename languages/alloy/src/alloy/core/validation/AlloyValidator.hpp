#pragma once

#include "alloy/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace alloy {
class AlloyValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkModule(const ast::Module &, cursiv::ValidationAcceptor &) const;
  void checkSignature(const ast::SigDecl &, cursiv::ValidationAcceptor &) const;
  void checkDeclaration(const ast::Decl &, cursiv::ValidationAcceptor &) const;
  void checkEnumeration(const ast::EnumDecl &,
                        cursiv::ValidationAcceptor &) const;
  void checkNumber(const ast::NumberExpr &, cursiv::ValidationAcceptor &) const;
  void checkString(const ast::StringExpr &, cursiv::ValidationAcceptor &) const;
  void checkOverallScope(const ast::Scope &,
                         cursiv::ValidationAcceptor &) const;
  void checkCommand(const ast::CmdDecl &, cursiv::ValidationAcceptor &) const;
  void checkScope(const ast::TypeScope &, cursiv::ValidationAcceptor &) const;
};
struct AlloyCoreServices;
void register_validation_checks(AlloyCoreServices &services);
} // namespace alloy
