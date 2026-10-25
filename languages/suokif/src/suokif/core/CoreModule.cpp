// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "suokif/core/CoreServices.hpp"
#include "suokif/core/validation/SuokifValidator.hpp"
#include "suokif/generated/Suokif/Module.hpp"

namespace suokif {

void install_core_module(SuokifCoreServices& services) {
  suokif::generated::Suokif::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace suokif
