// The checks of this language: what the grammar accepts but the language
// does not. Strict mode is one: real JS gates eight words and the legacy
// octal on whether the code is strict; here the grammar reads them
// everywhere and the code's own strictness judges after.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "javascript/core/ast.hpp"

namespace javascript {

class JavaScriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  // `0755` in strict code.
  void checkLegacyOctal(const ast::NumberLit& lit,
                        cursiv::ValidationAcceptor& accept) const;
  void checkNames(const ast::Program&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkTemplate(const ast::TemplateLit&, cursiv::ValidationAcceptor&) const;
  void checkRegex(const ast::RegexLit&, cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionDecl&, cursiv::ValidationAcceptor&) const;
  void checkFunctionExpr(const ast::FunctionExpr&, cursiv::ValidationAcceptor&) const;
  void checkArrow(const ast::ArrowFunctionExpr&, cursiv::ValidationAcceptor&) const;
  void checkProperty(const ast::Prop&, cursiv::ValidationAcceptor&) const;
  void checkMember(const ast::ClassMember&, cursiv::ValidationAcceptor&) const;
  void checkWith(const ast::WithStmt&, cursiv::ValidationAcceptor&) const;
  void checkNewTarget(const ast::NewTargetExpr&, cursiv::ValidationAcceptor&) const;
};

struct JavaScriptCoreServices;
void register_validation_checks(JavaScriptCoreServices& services);

}  // namespace javascript
