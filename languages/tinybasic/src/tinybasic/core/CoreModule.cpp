// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "tinybasic/core/CoreServices.hpp"
#include "tinybasic/core/validation/TinybasicValidator.hpp"
#include "tinybasic/generated/Tinybasic/Module.hpp"

namespace tinybasic {

void install_core_module(TinybasicCoreServices& services) {
  tinybasic::generated::Tinybasic::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace tinybasic
