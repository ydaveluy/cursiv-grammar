// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "iso8601/core/CoreServices.hpp"
#include "iso8601/core/validation/Iso8601Validator.hpp"
#include "iso8601/generated/Iso8601/Module.hpp"

namespace iso8601 {

void install_core_module(Iso8601CoreServices& services) {
  iso8601::generated::Iso8601::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace iso8601
