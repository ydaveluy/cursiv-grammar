// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "xml/core/CoreServices.hpp"
#include "xml/core/validation/XmlValidator.hpp"
#include "xml/generated/Xml/Module.hpp"

namespace xml {

void install_core_module(XmlCoreServices& services) {
  xml::generated::Xml::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace xml
