// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "url/core/CoreServices.hpp"
#include "url/core/validation/UrlValidator.hpp"
#include "url/generated/Url/Module.hpp"

namespace url {

void install_core_module(UrlCoreServices& services) {
  url::generated::Url::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace url
