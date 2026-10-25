// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "json/core/CoreServices.hpp"
#include "json/core/validation/JsonValidator.hpp"
#include "json/generated/Json/Module.hpp"

namespace json {

void install_core_module(JsonCoreServices& services) {
  json::generated::Json::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace json
