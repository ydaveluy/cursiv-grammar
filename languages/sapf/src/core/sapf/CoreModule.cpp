// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sapf/CoreServices.hpp"
#include "sapf/validation/SapfValidator.hpp"
#include "sapf/generated/Sapf/Module.hpp"

namespace sapf {

void install_core_module(SapfCoreServices& services) {
  sapf::generated::Sapf::installGeneratedModule(services);
  register_validation_checks(services);
}

}  // namespace sapf
