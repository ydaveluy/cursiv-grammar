#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "oncrpc/core/ast.hpp"

namespace oncrpc {

class OncrpcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkUniqueNames(const ast::Specification& spec,
                        cursiv::ValidationAcceptor& accept) const;
  void checkDeclaration(const ast::ValueDecl&, cursiv::ValidationAcceptor&) const;
  void checkVoid(const ast::VoidDecl&, cursiv::ValidationAcceptor&) const;
  void checkConstant(const ast::ConstantDef&, cursiv::ValidationAcceptor&) const;
  void checkEnumConstant(const ast::EnumConst&, cursiv::ValidationAcceptor&) const;
  void checkCase(const ast::CaseLabel&, cursiv::ValidationAcceptor&) const;
  void checkProcedure(const ast::ProcedureDef&, cursiv::ValidationAcceptor&) const;
  void checkVersion(const ast::VersionDef&, cursiv::ValidationAcceptor&) const;
  void checkProgram(const ast::ProgramDef&, cursiv::ValidationAcceptor&) const;
  void checkTypedef(const ast::TypedefDef&, cursiv::ValidationAcceptor&) const;
};

struct OncrpcCoreServices;
void register_validation_checks(OncrpcCoreServices& services);

}  // namespace oncrpc
