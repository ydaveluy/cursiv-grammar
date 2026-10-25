// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "parkingsign/core/CoreServices.hpp"
#include "parkingsign/core/validation/ParkingsignValidator.hpp"
#include "parkingsign/generated/Parkingsign/Module.hpp"

namespace parkingsign {

void install_core_module(ParkingsignCoreServices& services) {
  parkingsign::generated::Parkingsign::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace parkingsign
