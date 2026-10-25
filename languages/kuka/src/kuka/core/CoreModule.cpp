// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "kuka/core/CoreServices.hpp"
#include "kuka/core/validation/KukaValidator.hpp"
#include "kuka/generated/Kuka/Module.hpp"

namespace kuka {

void install_core_module(KukaCoreServices& services) {
  kuka::generated::Kuka::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace kuka
