#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pike/core/ast.hpp"

namespace pike {

class PikeValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberLit &, cursiv::ValidationAcceptor &) const;
};

struct PikeCoreServices;
void register_validation_checks(PikeCoreServices &services);

} // namespace pike
