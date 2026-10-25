#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "tnt/core/ast.hpp"

namespace tnt {
class TntValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkTerm(const ast::Term&, cursiv::ValidationAcceptor&) const;
  void checkCompound(const ast::LogicalExpression&, cursiv::ValidationAcceptor&) const;
};
struct TntCoreServices;
void register_validation_checks(TntCoreServices& services);
}  // namespace tnt
