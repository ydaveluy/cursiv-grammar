// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "srt/core/CoreServices.hpp"
#include "srt/core/validation/SrtValidator.hpp"
#include "srt/generated/Srt/Module.hpp"

namespace srt {

void install_core_module(SrtCoreServices& services) {
  srt::generated::Srt::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace srt
