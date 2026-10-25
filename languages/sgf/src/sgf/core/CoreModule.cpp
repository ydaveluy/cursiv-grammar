// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sgf/core/CoreServices.hpp"
#include "sgf/core/validation/SgfValidator.hpp"
#include "sgf/generated/Sgf/Module.hpp"

namespace sgf {

void install_core_module(SgfCoreServices& services) {
  sgf::generated::Sgf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sgf
