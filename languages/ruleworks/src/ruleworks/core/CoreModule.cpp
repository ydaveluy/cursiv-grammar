// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ruleworks/core/CoreServices.hpp"
#include "ruleworks/core/validation/RuleworksValidator.hpp"
#include "ruleworks/generated/Ruleworks/Module.hpp"

namespace ruleworks {

void install_core_module(RuleworksCoreServices& services) {
  ruleworks::generated::Ruleworks::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ruleworks
