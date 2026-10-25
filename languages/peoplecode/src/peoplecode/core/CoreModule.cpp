// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "peoplecode/core/CoreServices.hpp"
#include "peoplecode/core/validation/PeoplecodeValidator.hpp"
#include "peoplecode/generated/Peoplecode/Module.hpp"

namespace peoplecode {

void install_core_module(PeoplecodeCoreServices& services) {
  peoplecode::generated::Peoplecode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace peoplecode
