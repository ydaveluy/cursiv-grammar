// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "hexpat/core/CoreServices.hpp"
#include "hexpat/core/validation/HexpatValidator.hpp"
#include "hexpat/generated/Hexpat/Module.hpp"

namespace hexpat {

void install_core_module(HexpatCoreServices& services) {
  hexpat::generated::Hexpat::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace hexpat
