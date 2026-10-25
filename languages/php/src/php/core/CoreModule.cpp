// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "php/core/CoreServices.hpp"
#include "php/core/validation/PhpValidator.hpp"
#include "php/generated/Php/Module.hpp"

namespace php {

void install_core_module(PhpCoreServices& services) {
  php::generated::Php::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace php
