#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "apt/core/ast.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"

namespace apt {

class AptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRecord(const ast::Record&, cursiv::ValidationAcceptor&) const;
};

struct AptCoreServices;
void register_validation_checks(AptCoreServices& services);

}  // namespace apt
