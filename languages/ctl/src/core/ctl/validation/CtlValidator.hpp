#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "ctl/ast.hpp"

namespace ctl {
class CtlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkFormula(const ast::Proposition&, cursiv::ValidationAcceptor&) const;
};
struct CtlCoreServices;
void register_validation_checks(CtlCoreServices& services);
}  // namespace ctl
