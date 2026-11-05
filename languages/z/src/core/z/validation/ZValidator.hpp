// Model/type constraints and custom-operator binding are not implemented here.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "z/ast.hpp"

namespace z {

class ZValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ZCoreServices;
void register_validation_checks(ZCoreServices& services);

}  // namespace z
