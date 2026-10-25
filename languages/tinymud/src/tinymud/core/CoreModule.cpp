// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tinymud/core/CoreServices.hpp"
#include "tinymud/core/validation/TinymudValidator.hpp"
#include "tinymud/generated/Tinymud/Module.hpp"

namespace tinymud {

void install_core_module(TinymudCoreServices& services) {
  tinymud::generated::Tinymud::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tinymud
