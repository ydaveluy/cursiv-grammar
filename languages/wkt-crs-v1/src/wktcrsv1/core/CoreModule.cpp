// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wktcrsv1/core/CoreServices.hpp"
#include "wktcrsv1/core/validation/WktCrsV1Validator.hpp"
#include "wktcrsv1/generated/WktCrsV1/Module.hpp"

namespace wktcrsv1 {

void install_core_module(WktCrsV1CoreServices& services) {
  wktcrsv1::generated::WktCrsV1::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wktcrsv1
