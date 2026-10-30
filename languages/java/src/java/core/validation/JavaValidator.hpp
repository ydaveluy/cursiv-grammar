// The checks of this language: what the grammar accepts but the language does
// not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "java/core/ast.hpp"

namespace java {

class JavaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkRecordComponentVarargsIsLast(const ast::RecordDecl& record,
                                         cursiv::ValidationAcceptor& accept) const;
  void checkMethod(const ast::MethodDecl&, cursiv::ValidationAcceptor&) const;
  void checkConstructor(const ast::ConstructorDecl&, cursiv::ValidationAcceptor&) const;
  void checkLambda(const ast::LambdaExpr&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::CallExpr&, cursiv::ValidationAcceptor&) const;
  void checkGenericCall(const ast::ExplicitGenericInvokeExpr&, cursiv::ValidationAcceptor&) const;
  void checkPattern(const ast::TypePattern&, cursiv::ValidationAcceptor&) const;
};

struct JavaCoreServices;
void register_validation_checks(JavaCoreServices& services);

}  // namespace java
