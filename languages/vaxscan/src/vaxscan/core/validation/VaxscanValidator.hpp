// The checks of this language: what the grammar accepts but the language
// does not.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "vaxscan/core/ast.hpp"

namespace vaxscan {

class VaxscanValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct VaxscanCoreServices;
void register_validation_checks(VaxscanCoreServices& services);

}  // namespace vaxscan
