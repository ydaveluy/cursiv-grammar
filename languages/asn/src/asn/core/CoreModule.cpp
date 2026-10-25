// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "asn/core/CoreServices.hpp"
#include "asn/core/validation/AsnValidator.hpp"
#include "asn/generated/Asn/Module.hpp"

namespace asn {

void install_core_module(AsnCoreServices& services) {
  asn::generated::Asn::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace asn
