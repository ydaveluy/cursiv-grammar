// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "pcre/core/CoreServices.hpp"
#include "pcre/core/validation/PcreValidator.hpp"
#include "pcre/generated/Pcre/Module.hpp"

namespace pcre {

void install_core_module(PcreCoreServices& services) {
  pcre::generated::Pcre::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace pcre
