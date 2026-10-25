// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "dcm/core/CoreServices.hpp"
#include "dcm/core/validation/DcmValidator.hpp"
#include "dcm/generated/Dcm/Module.hpp"

namespace dcm {

void install_core_module(DcmCoreServices& services) {
  dcm::generated::Dcm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace dcm
