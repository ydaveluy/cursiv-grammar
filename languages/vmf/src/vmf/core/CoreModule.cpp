// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vmf/core/CoreServices.hpp"
#include "vmf/core/validation/VmfValidator.hpp"
#include "vmf/generated/Vmf/Module.hpp"

namespace vmf {

void install_core_module(VmfCoreServices& services) {
  vmf::generated::Vmf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace vmf
