#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "pike/ast.hpp"

namespace pike {

class PikeValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkNumber(const ast::NumberLit &, cursiv::ValidationAcceptor &) const;
};

struct PikeCoreServices;
void register_validation_checks(PikeCoreServices &services);

} // namespace pike
