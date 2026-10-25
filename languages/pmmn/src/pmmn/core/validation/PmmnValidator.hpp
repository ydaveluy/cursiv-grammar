// The checks of this language: what the grammar accepts but the language does
// not. None yet -- every program the grammar accepts is valid.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "pmmn/core/ast.hpp"

namespace pmmn {

class PmmnValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct PmmnCoreServices;
void register_validation_checks(PmmnCoreServices& services);

}  // namespace pmmn
