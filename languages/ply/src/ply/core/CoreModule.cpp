// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "ply/core/CoreServices.hpp"
#include "ply/core/validation/PlyValidator.hpp"
#include "ply/generated/Ply/Module.hpp"

namespace ply {

void install_core_module(PlyCoreServices& services) {
  ply::generated::Ply::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace ply
