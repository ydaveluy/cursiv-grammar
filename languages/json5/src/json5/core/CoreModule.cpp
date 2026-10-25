// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "json5/core/CoreServices.hpp"
#include "json5/core/validation/Json5Validator.hpp"
#include "json5/generated/Json5/Module.hpp"

namespace json5 {

void install_core_module(Json5CoreServices& services) {
  json5::generated::Json5::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace json5
