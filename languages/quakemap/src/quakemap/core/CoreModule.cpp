// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "quakemap/core/CoreServices.hpp"
#include "quakemap/core/validation/QuakemapValidator.hpp"
#include "quakemap/generated/Quakemap/Module.hpp"

namespace quakemap {

void install_core_module(QuakemapCoreServices& services) {
  quakemap::generated::Quakemap::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace quakemap
