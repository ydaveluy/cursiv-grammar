// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "bencoding/core/CoreServices.hpp"
#include "bencoding/core/validation/BencodingValidator.hpp"
#include "bencoding/generated/Bencoding/Module.hpp"

namespace bencoding {

void install_core_module(BencodingCoreServices& services) {
  bencoding::generated::Bencoding::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace bencoding
