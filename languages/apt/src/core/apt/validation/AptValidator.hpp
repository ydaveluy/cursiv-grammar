#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "apt/ast.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"

namespace apt {

class AptValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkRecord(const ast::Record&, cursiv::ValidationAcceptor&) const;
};

struct AptCoreServices;
void register_validation_checks(AptCoreServices& services);

}  // namespace apt
