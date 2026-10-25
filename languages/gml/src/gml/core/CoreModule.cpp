// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "gml/core/CoreServices.hpp"
#include "gml/core/validation/GmlValidator.hpp"
#include "gml/generated/Gml/Module.hpp"

namespace gml {

void install_core_module(GmlCoreServices& services) {
  gml::generated::Gml::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace gml
