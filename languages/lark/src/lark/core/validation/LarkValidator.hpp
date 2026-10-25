#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "lark/core/ast.hpp"

namespace lark {

class LarkValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkDeclaration(const ast::Declaration&, cursiv::ValidationAcceptor&) const;
  void checkRepeat(const ast::Expr&, cursiv::ValidationAcceptor&) const;
  void checkAlias(const ast::Alias&, cursiv::ValidationAcceptor&) const;
  void checkTemplate(const ast::TemplateUsage&, cursiv::ValidationAcceptor&) const;
  void checkLiteral(const ast::Literal&, cursiv::ValidationAcceptor&) const;
};

struct LarkCoreServices;
void register_validation_checks(LarkCoreServices& services);

}  // namespace lark
