// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tinyosnesc/core/CoreServices.hpp"
#include "tinyosnesc/core/validation/TinyosNescValidator.hpp"
#include "tinyosnesc/generated/TinyosNesc/Module.hpp"

namespace tinyosnesc {

void install_core_module(TinyosNescCoreServices& services) {
  tinyosnesc::generated::TinyosNesc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tinyosnesc
