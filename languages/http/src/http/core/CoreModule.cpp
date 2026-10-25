// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "http/core/CoreServices.hpp"
#include "http/core/validation/HttpValidator.hpp"
#include "http/generated/Http/Module.hpp"

namespace http {

void install_core_module(HttpCoreServices& services) {
  http::generated::Http::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace http
