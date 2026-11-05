#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "typescript/ast.hpp"

namespace typescript {

class TypeScriptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkObjectMembers(const ast::ObjectType& object,
                          cursiv::ValidationAcceptor& accept) const;
  void checkInterfaceMembers(const ast::InterfaceDecl& interface,
                             cursiv::ValidationAcceptor& accept) const;
  void checkResources(const ast::VarDecl& declaration,
                      cursiv::ValidationAcceptor& accept) const;
  void checkIdentifierNames(const cursiv::AstNode&, cursiv::ValidationAcceptor&) const;
  void checkLabel(const ast::LabeledStmt&, cursiv::ValidationAcceptor&) const;
  void checkBreak(const ast::BreakStmt&, cursiv::ValidationAcceptor&) const;
  void checkContinue(const ast::ContinueStmt&, cursiv::ValidationAcceptor&) const;
  void checkImportBinding(const ast::ImportSpecifier&, cursiv::ValidationAcceptor&) const;
  void checkImportEquals(const ast::ImportEqualsDecl&, cursiv::ValidationAcceptor&) const;
};

struct TypeScriptCoreServices;
void register_validation_checks(TypeScriptCoreServices& services);

}  // namespace typescript
