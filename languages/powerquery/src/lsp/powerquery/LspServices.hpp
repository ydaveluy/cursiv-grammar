// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "powerquery/CoreServices.hpp"

namespace powerquery {

struct PowerquerySharedServices : PowerquerySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PowerqueryServices final
    : cursiv::LspServicesFor<PowerqueryCoreServices, PowerquerySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace powerquery
