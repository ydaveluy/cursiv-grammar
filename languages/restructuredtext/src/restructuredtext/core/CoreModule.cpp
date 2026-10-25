// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "restructuredtext/core/CoreServices.hpp"
#include "restructuredtext/core/validation/RestructuredtextValidator.hpp"
#include "restructuredtext/generated/Restructuredtext/Module.hpp"

namespace restructuredtext {

void install_core_module(RestructuredtextCoreServices& services) {
  restructuredtext::generated::Restructuredtext::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace restructuredtext
