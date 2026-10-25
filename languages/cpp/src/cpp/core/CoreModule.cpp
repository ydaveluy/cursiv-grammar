// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "cpp/core/CoreServices.hpp"
#include "cpp/core/validation/CppValidator.hpp"
#include "cpp/generated/Cpp/Module.hpp"

namespace cpp {

void install_core_module(CppCoreServices& services) {
  cpp::generated::Cpp::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace cpp
