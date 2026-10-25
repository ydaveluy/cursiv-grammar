// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "lolcode/core/CoreServices.hpp"
#include "lolcode/core/validation/LolcodeValidator.hpp"
#include "lolcode/generated/Lolcode/Module.hpp"

namespace lolcode {

void install_core_module(LolcodeCoreServices& services) {
  lolcode::generated::Lolcode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace lolcode
