// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "sexpression/core/CoreServices.hpp"
#include "sexpression/core/validation/SexpressionValidator.hpp"
#include "sexpression/generated/Sexpression/Module.hpp"

namespace sexpression {

void install_core_module(SexpressionCoreServices& services) {
  sexpression::generated::Sexpression::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace sexpression
