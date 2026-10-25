// The checks of this language: none needed beyond what the grammar and the
// default reference resolution already enforce.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "scotty/core/ast.hpp"

namespace scotty {

class ScottyValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct ScottyCoreServices;
void register_validation_checks(ScottyCoreServices& services);

}  // namespace scotty
