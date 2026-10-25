// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "wkt/core/CoreServices.hpp"
#include "wkt/core/validation/WktValidator.hpp"
#include "wkt/generated/Wkt/Module.hpp"

namespace wkt {

void install_core_module(WktCoreServices& services) {
  wkt::generated::Wkt::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace wkt
