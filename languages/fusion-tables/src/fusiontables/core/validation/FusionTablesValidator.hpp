// The checks of this language: none yet -- the grammar is purely syntactic,
// with no symbol table or cross-reference to validate.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace fusiontables {

class FusionTablesValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct FusionTablesCoreServices;
void register_validation_checks(FusionTablesCoreServices& services);

}  // namespace fusiontables
