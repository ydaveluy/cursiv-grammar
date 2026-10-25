// The checks of this language. None yet: the grammar accepts what the
// language accepts, with nothing left for a validator to police.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "focal/core/ast.hpp"

namespace focal {

class FocalValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FocalCoreServices;
void register_validation_checks(FocalCoreServices& services);

}  // namespace focal
