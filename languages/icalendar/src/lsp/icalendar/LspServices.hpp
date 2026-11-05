// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "icalendar/CoreServices.hpp"

namespace icalendar {

struct IcalendarSharedServices : IcalendarSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct IcalendarServices final
    : cursiv::LspServicesFor<IcalendarCoreServices, IcalendarSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace icalendar
