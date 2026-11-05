// The checks of this language: none yet, beyond what the grammar accepts.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "spl/ast.hpp"

namespace spl {

class SplValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct SplCoreServices;
void register_validation_checks(SplCoreServices& services);

}  // namespace spl
