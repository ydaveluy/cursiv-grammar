// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "guitartab/core/CoreServices.hpp"
#include "guitartab/core/validation/GuitartabValidator.hpp"
#include "guitartab/generated/Guitartab/Module.hpp"

namespace guitartab {

void install_core_module(GuitartabCoreServices& services) {
  guitartab::generated::Guitartab::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace guitartab
