// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "yara/core/CoreServices.hpp"
#include "yara/core/validation/YaraValidator.hpp"
#include "yara/generated/Yara/Module.hpp"

namespace yara {

void install_core_module(YaraCoreServices& services) {
  yara::generated::Yara::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace yara
