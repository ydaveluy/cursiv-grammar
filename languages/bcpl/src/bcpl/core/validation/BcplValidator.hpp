// The checks of this language: none yet -- nothing here needs a check
// beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "bcpl/core/ast.hpp"

namespace bcpl {

class BcplValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct BcplCoreServices;
void register_validation_checks(BcplCoreServices& services);

}  // namespace bcpl
