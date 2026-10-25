// The checks of this language. None yet: nothing here needs a check beyond
// parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "ocl/core/ast.hpp"

namespace ocl {

class OclValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct OclCoreServices;
void register_validation_checks(OclCoreServices& services);

}  // namespace ocl
