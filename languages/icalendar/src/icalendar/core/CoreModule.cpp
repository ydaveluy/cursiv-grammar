// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "icalendar/core/CoreServices.hpp"
#include "icalendar/core/validation/IcalendarValidator.hpp"
#include "icalendar/generated/Icalendar/Module.hpp"

namespace icalendar {

void install_core_module(IcalendarCoreServices& services) {
  icalendar::generated::Icalendar::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace icalendar
