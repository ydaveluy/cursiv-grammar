// Local Refal-5 identifier and macrodigit restrictions.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "refal/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace refal {

class RefalValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberSymbol&, cursiv::ValidationAcceptor&) const;
  void checkFunction(const ast::FunctionDef&, cursiv::ValidationAcceptor&) const;
  void checkExternal(const ast::ExternalDecl&, cursiv::ValidationAcceptor&) const;
  void checkVariable(const ast::Variable&, cursiv::ValidationAcceptor&) const;
  void checkCall(const ast::Call&, cursiv::ValidationAcceptor&) const;
};

struct RefalCoreServices;
void register_validation_checks(RefalCoreServices& services);

}  // namespace refal
