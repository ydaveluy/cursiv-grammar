// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "desktopentry/CoreServices.hpp"
#include "desktopentry/validation/DesktopEntryValidator.hpp"
#include "desktopentry/generated/DesktopEntry/Module.hpp"

namespace desktopentry {

void install_core_module(DesktopEntryCoreServices& services) {
  desktopentry::generated::DesktopEntry::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace desktopentry
