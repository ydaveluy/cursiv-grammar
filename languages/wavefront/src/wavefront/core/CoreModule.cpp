// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wavefront/core/CoreServices.hpp"
#include "wavefront/core/validation/WavefrontValidator.hpp"
#include "wavefront/generated/Wavefront/Module.hpp"

namespace wavefront {

void install_core_module(WavefrontCoreServices& services) {
  wavefront::generated::Wavefront::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wavefront
