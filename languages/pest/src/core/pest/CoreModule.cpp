// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pest/CoreServices.hpp"
#include "pest/validation/PestValidator.hpp"
#include "pest/generated/Pest/Module.hpp"

namespace pest {

void install_core_module(PestCoreServices& services) {
  pest::generated::Pest::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pest
