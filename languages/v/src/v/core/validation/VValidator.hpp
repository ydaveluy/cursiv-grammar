// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "v/core/ast.hpp"

namespace v {

class VValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct VCoreServices;
void register_validation_checks(VCoreServices& services);

}  // namespace v
