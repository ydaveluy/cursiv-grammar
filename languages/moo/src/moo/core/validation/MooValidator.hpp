// The checks of this language: none yet, beyond what the grammar itself
// already enforces by shape.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "moo/core/ast.hpp"

namespace moo {

class MooValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MooCoreServices;
void register_validation_checks(MooCoreServices& services);

}  // namespace moo
