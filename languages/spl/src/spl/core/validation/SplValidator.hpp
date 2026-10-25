// The checks of this language: none yet, beyond what the grammar accepts.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "spl/core/ast.hpp"

namespace spl {

class SplValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SplCoreServices;
void register_validation_checks(SplCoreServices& services);

}  // namespace spl
