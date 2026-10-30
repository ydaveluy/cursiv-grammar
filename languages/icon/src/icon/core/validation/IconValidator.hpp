// Native declaration constraints left outside the shared syntax rules.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "icon/core/ast.hpp"

namespace icon {

class IconValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkProcedure(const ast::Procedure&, cursiv::ValidationAcceptor&) const;
  void checkInvocable(const ast::InvocableItem&, cursiv::ValidationAcceptor&) const;
  void checkSuspend(const ast::SuspendExpr&, cursiv::ValidationAcceptor&) const;
};

struct IconCoreServices;
void register_validation_checks(IconCoreServices& services);

}  // namespace icon
