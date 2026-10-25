// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "toml/core/CoreServices.hpp"
#include "toml/core/validation/TomlValidator.hpp"
#include "toml/generated/Toml/Module.hpp"

namespace toml {

void install_core_module(TomlCoreServices& services) {
  toml::generated::Toml::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace toml
