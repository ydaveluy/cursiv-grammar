// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "asl/core/CoreServices.hpp"
#include "asl/core/validation/AslValidator.hpp"
#include "asl/generated/Asl/Module.hpp"

namespace asl {

void install_core_module(AslCoreServices& services) {
  asl::generated::Asl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace asl
