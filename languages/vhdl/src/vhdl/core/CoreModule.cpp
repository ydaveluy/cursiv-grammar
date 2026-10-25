// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "vhdl/core/CoreServices.hpp"
#include "vhdl/core/validation/VhdlValidator.hpp"
#include "vhdl/generated/Vhdl/Module.hpp"

namespace vhdl {

void install_core_module(VhdlCoreServices& services) {
  vhdl::generated::Vhdl::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace vhdl
