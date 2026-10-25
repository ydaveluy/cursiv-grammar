// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lark/core/CoreServices.hpp"
#include "lark/core/validation/LarkValidator.hpp"
#include "lark/generated/Lark/Module.hpp"

namespace lark {

void install_core_module(LarkCoreServices& services) {
  lark::generated::Lark::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lark
