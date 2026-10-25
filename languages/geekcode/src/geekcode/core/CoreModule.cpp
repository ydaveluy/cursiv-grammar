// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "geekcode/core/CoreServices.hpp"
#include "geekcode/core/validation/GeekcodeValidator.hpp"
#include "geekcode/generated/Geekcode/Module.hpp"

namespace geekcode {

void install_core_module(GeekcodeCoreServices& services) {
  geekcode::generated::Geekcode::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace geekcode
