// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "powerbuilderdw/core/CoreServices.hpp"
#include "powerbuilderdw/core/validation/PowerbuilderdwValidator.hpp"
#include "powerbuilderdw/generated/Powerbuilderdw/Module.hpp"

namespace powerbuilderdw {

void install_core_module(PowerbuilderdwCoreServices& services) {
  powerbuilderdw::generated::Powerbuilderdw::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace powerbuilderdw
