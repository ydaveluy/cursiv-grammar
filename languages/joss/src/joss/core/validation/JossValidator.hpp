// The checks of this language: none yet -- nothing in the grammar needs a
// check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "joss/core/ast.hpp"

namespace joss {

class JossValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct JossCoreServices;
void register_validation_checks(JossCoreServices& services);

}  // namespace joss
