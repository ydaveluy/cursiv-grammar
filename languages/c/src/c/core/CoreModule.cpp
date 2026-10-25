// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include <memory>

#include "c/core/CoreServices.hpp"
#include "c/core/references/CScopeComputation.hpp"
#include "c/core/validation/CValidator.hpp"
#include "c/generated/C/Module.hpp"

namespace c {

void install_core_module(CCoreServices& services) {
  c::generated::C::install_generated_module(services);
  services.scopeComputation = std::make_unique<CScopeComputation>(services);
  register_validation_checks(services);
}

}  // namespace c
