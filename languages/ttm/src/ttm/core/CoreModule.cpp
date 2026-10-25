// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ttm/core/CoreServices.hpp"
#include "ttm/core/validation/TtmValidator.hpp"
#include "ttm/generated/Ttm/Module.hpp"

namespace ttm {

void install_core_module(TtmCoreServices& services) {
  ttm::generated::Ttm::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ttm
