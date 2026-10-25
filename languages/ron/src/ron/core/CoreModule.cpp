// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ron/core/CoreServices.hpp"
#include "ron/core/validation/RonValidator.hpp"
#include "ron/generated/Ron/Module.hpp"

namespace ron {

void install_core_module(RonCoreServices& services) {
  ron::generated::Ron::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ron
