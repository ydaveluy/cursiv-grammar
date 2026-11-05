// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "wat/CoreServices.hpp"

namespace wat {

struct WatSharedServices : WatSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WatServices final
    : cursiv::LspServicesFor<WatCoreServices, WatSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace wat
