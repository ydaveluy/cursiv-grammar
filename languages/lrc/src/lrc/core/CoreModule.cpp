// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lrc/core/CoreServices.hpp"
#include "lrc/core/validation/LrcValidator.hpp"
#include "lrc/generated/Lrc/Module.hpp"

namespace lrc {

void install_core_module(LrcCoreServices& services) {
  lrc::generated::Lrc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lrc
