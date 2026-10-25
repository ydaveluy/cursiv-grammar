// The checks of this language: none -- the grammar's structure is the whole
// language, nothing else needs judging.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace molecule {

class MoleculeValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MoleculeCoreServices;
void register_validation_checks(MoleculeCoreServices& services);

}  // namespace molecule
