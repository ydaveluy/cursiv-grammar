#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "stringtemplate/core/ast.hpp"

namespace stringtemplate {

class StringtemplateValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkArguments(const ast::ArgList &, cursiv::ValidationAcceptor &) const;
  void checkMapping(const ast::MapExpr &, cursiv::ValidationAcceptor &) const;
  void checkStage(const ast::MapStage &, cursiv::ValidationAcceptor &) const;
  void checkOption(const ast::Option &, cursiv::ValidationAcceptor &) const;
};

struct StringtemplateCoreServices;
void register_validation_checks(StringtemplateCoreServices &services);

} // namespace stringtemplate
