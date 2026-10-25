// The checks of this language: none yet -- the grammar's own constructs
// carry no invariant a single pass over the tree can usefully police.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace cayenne {

class CayenneValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct CayenneCoreServices;
void register_validation_checks(CayenneCoreServices& services);

}  // namespace cayenne
