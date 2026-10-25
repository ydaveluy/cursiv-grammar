// The checks of this language. None yet: the grammar alone captures maml.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"

namespace maml {

class MamlValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct MamlCoreServices;
void register_validation_checks(MamlCoreServices& services);

}  // namespace maml
