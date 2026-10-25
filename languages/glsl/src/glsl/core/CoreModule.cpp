// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "glsl/core/CoreServices.hpp"
#include "glsl/core/validation/GlslValidator.hpp"
#include "glsl/generated/Glsl/Module.hpp"

namespace glsl {

void install_core_module(GlslCoreServices& services) {
  glsl::generated::Glsl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace glsl
