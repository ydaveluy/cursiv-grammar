// Local static checks that CPython performs while parsing or compiling.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "python/core/ast.hpp"

namespace python {

class PythonValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkParams(const ast::Params&, cursiv::ValidationAcceptor&) const;
  void checkStr(const ast::Str&, cursiv::ValidationAcceptor&) const;
  void checkStrings(const ast::Strings&, cursiv::ValidationAcceptor&) const;
  void checkAssign(const ast::AssignStmt&, cursiv::ValidationAcceptor&) const;
  void checkAugAssign(const ast::AugAssignStmt&, cursiv::ValidationAcceptor&) const;
  void checkAnnAssign(const ast::AnnAssignStmt&, cursiv::ValidationAcceptor&) const;
  void checkDel(const ast::DelStmt&, cursiv::ValidationAcceptor&) const;
  void checkFor(const ast::ForStmt&, cursiv::ValidationAcceptor&) const;
  void checkComprehension(const ast::Comprehension&, cursiv::ValidationAcceptor&) const;
  void checkWithItem(const ast::WithItem&, cursiv::ValidationAcceptor&) const;
  void checkTry(const ast::TryStmt&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::CallExpr&, cursiv::ValidationAcceptor&) const;
  void checkExprStmt(const ast::ExprStmt&, cursiv::ValidationAcceptor&) const;
  void checkMatchValue(const ast::MatchValue&, cursiv::ValidationAcceptor&) const;
};

struct PythonCoreServices;
void register_validation_checks(PythonCoreServices& services);

}  // namespace python
