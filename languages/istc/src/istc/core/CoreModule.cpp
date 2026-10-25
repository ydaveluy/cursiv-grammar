// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "istc/core/CoreServices.hpp"
#include "istc/core/validation/IstcValidator.hpp"
#include "istc/generated/Istc/Module.hpp"

namespace istc {

void install_core_module(IstcCoreServices& services) {
  istc::generated::Istc::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace istc
