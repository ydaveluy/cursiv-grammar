// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "kotlin/core/CoreServices.hpp"
#include "kotlin/core/validation/KotlinValidator.hpp"
#include "kotlin/generated/Kotlin/Module.hpp"

namespace kotlin {

void install_core_module(KotlinCoreServices& services) {
  kotlin::generated::Kotlin::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace kotlin
