// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sysmlv2/core/CoreServices.hpp"
#include "sysmlv2/core/validation/SysmlV2Validator.hpp"
#include "sysmlv2/generated/SysmlV2/Module.hpp"

namespace sysmlv2 {

void install_core_module(SysmlV2CoreServices& services) {
  sysmlv2::generated::SysmlV2::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sysmlv2
