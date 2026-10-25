// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "semver/core/CoreServices.hpp"
#include "semver/core/validation/SemverValidator.hpp"
#include "semver/generated/Semver/Module.hpp"

namespace semver {

void install_core_module(SemverCoreServices& services) {
  semver::generated::Semver::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace semver
