#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "rpn/core/ast.hpp"

namespace rpn {

class RpnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RpnCoreServices;
void register_validation_checks(RpnCoreServices& services);

}  // namespace rpn
