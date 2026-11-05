// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "propcalc/CoreServices.hpp"

namespace propcalc {

struct PropcalcSharedServices : PropcalcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PropcalcServices final
    : cursiv::LspServicesFor<PropcalcCoreServices, PropcalcSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace propcalc
