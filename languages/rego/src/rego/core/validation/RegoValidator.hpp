#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rego/core/ast.hpp"

namespace rego {
class RegoValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberLiteral&,
                   cursiv::ValidationAcceptor&) const;
  void checkScalar(const ast::ScalarTerm&, cursiv::ValidationAcceptor&) const;
  void checkQuery(const ast::Query&, cursiv::ValidationAcceptor&) const;
  void checkLiteral(const ast::LiteralExpr&, cursiv::ValidationAcceptor&) const;
  void checkSome(const ast::SomeDecl&, cursiv::ValidationAcceptor&) const;
  void checkEvery(const ast::Every&, cursiv::ValidationAcceptor&) const;
  void checkRule(const ast::Rule&, cursiv::ValidationAcceptor&) const;
  void checkQueryStmt(const ast::QueryStmt&, cursiv::ValidationAcceptor&) const;
  void checkPackage(const ast::PackageDecl&, cursiv::ValidationAcceptor&) const;
  void checkImport(const ast::ImportDecl&, cursiv::ValidationAcceptor&) const;
  void checkRef(const ast::Ref&, cursiv::ValidationAcceptor&) const;
  void checkAccess(const ast::AccessExpr&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::Call&, cursiv::ValidationAcceptor&) const;
  void checkWith(const ast::WithMod&, cursiv::ValidationAcceptor&) const;
};
struct RegoCoreServices;
void register_validation_checks(RegoCoreServices& services);
}  // namespace rego
