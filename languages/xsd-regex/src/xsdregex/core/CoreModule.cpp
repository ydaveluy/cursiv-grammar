// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "xsdregex/core/CoreServices.hpp"
#include "xsdregex/core/validation/XsdRegexValidator.hpp"
#include "xsdregex/generated/XsdRegex/Module.hpp"

namespace xsdregex {

void install_core_module(XsdRegexCoreServices& services) {
  xsdregex::generated::XsdRegex::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace xsdregex
