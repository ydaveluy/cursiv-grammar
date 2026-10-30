// Model/type constraints and custom-operator binding are not implemented here.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "z/core/ast.hpp"

namespace z {

class ZValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ZCoreServices;
void register_validation_checks(ZCoreServices& services);

}  // namespace z
