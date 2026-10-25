// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "python/core/CoreServices.hpp"
#include "python/core/validation/PythonValidator.hpp"
#include "python/generated/Python/Module.hpp"

namespace python {

void install_core_module(PythonCoreServices& services) {
  python::generated::Python::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace python
