// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include <memory>

#include "alef/core/CoreServices.hpp"
#include "alef/core/references/AlefScopeComputation.hpp"
#include "alef/core/validation/AlefValidator.hpp"
#include "alef/generated/Alef/Module.hpp"

namespace alef {

void install_core_module(AlefCoreServices& services) {
  alef::generated::Alef::install_generated_module(services);
  services.scopeComputation = std::make_unique<AlefScopeComputation>(services);
  register_validation_checks(services);
}

}  // namespace alef
