// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "clu/core/CoreServices.hpp"
#include "clu/core/validation/CluValidator.hpp"
#include "clu/generated/Clu/Module.hpp"

namespace clu {

void install_core_module(CluCoreServices& services) {
  clu::generated::Clu::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace clu
