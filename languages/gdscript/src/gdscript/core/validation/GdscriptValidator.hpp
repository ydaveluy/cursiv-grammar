#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "gdscript/core/ast.hpp"

namespace gdscript {
class GdscriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkProgram(const ast::Program&, cursiv::ValidationAcceptor&) const;
  void checkMethod(const ast::MethodDecl&, cursiv::ValidationAcceptor&) const;
  void checkString(const ast::StringLit&, cursiv::ValidationAcceptor&) const;
  void checkPath(const ast::NodePathLit&, cursiv::ValidationAcceptor&) const;
  void checkNumber(const ast::FloatLit&, cursiv::ValidationAcceptor&) const;
  void checkInteger(const ast::IntLit&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::CallExpr&, cursiv::ValidationAcceptor&) const;
  void checkBreak(const ast::BreakStmt&, cursiv::ValidationAcceptor&) const;
  void checkContinue(const ast::ContinueStmt&,
                     cursiv::ValidationAcceptor&) const;
};
struct GdscriptCoreServices;
void register_validation_checks(GdscriptCoreServices&);
}  // namespace gdscript
