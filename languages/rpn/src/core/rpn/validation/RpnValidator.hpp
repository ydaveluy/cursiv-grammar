#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "rpn/ast.hpp"

namespace rpn {

class RpnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct RpnCoreServices;
void register_validation_checks(RpnCoreServices& services);

}  // namespace rpn
