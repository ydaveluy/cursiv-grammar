#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "objc/core/ast.hpp"

namespace objc {
class ObjcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkGeneric(const ast::GenericExpr&, cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::NumberLit&, cursiv::ValidationAcceptor&) const;
  void checkCharacter(const ast::CharLit&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkAvailable(const ast::AvailableExpr&,
                      cursiv::ValidationAcceptor&) const;
  void checkTry(const ast::TryStmt&, cursiv::ValidationAcceptor&) const;
  void checkThrow(const ast::ThrowStmt&, cursiv::ValidationAcceptor&) const;
};
struct ObjcCoreServices;
void register_validation_checks(ObjcCoreServices& services);
}  // namespace objc
