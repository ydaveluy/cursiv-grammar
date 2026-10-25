// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "smtlibv2/core/CoreServices.hpp"
#include "smtlibv2/core/validation/Smtlibv2Validator.hpp"
#include "smtlibv2/generated/Smtlibv2/Module.hpp"

namespace smtlibv2 {

void install_core_module(Smtlibv2CoreServices& services) {
  smtlibv2::generated::Smtlibv2::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace smtlibv2
