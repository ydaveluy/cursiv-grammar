// The checks of this language beyond what the grammar already enforces.
#pragma once

#include "ada/core/ast.hpp"
#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace ada {

class AdaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkLogicalChain(const ast::BinaryExpr &,
                         cursiv::ValidationAcceptor &) const;
};

struct AdaCoreServices;
void register_validation_checks(AdaCoreServices& services);

}  // namespace ada
