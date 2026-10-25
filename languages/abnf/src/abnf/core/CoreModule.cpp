// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "abnf/core/CoreServices.hpp"
#include "abnf/core/validation/AbnfValidator.hpp"
#include "abnf/generated/Abnf/Module.hpp"

namespace abnf {

void install_core_module(AbnfCoreServices& services) {
  abnf::generated::Abnf::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace abnf
