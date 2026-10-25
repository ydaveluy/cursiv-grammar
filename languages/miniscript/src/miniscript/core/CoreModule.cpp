// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "miniscript/core/CoreServices.hpp"
#include "miniscript/core/validation/MiniscriptValidator.hpp"
#include "miniscript/generated/Miniscript/Module.hpp"

namespace miniscript {

void install_core_module(MiniscriptCoreServices& services) {
  miniscript::generated::Miniscript::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace miniscript
