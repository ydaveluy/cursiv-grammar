// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "limbo/core/CoreServices.hpp"
#include "limbo/core/validation/LimboValidator.hpp"
#include "limbo/generated/Limbo/Module.hpp"

namespace limbo {

void install_core_module(LimboCoreServices& services) {
  limbo::generated::Limbo::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace limbo
