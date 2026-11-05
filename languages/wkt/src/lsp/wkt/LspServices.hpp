// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "wkt/CoreServices.hpp"

namespace wkt {

struct WktSharedServices : WktSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WktServices final
    : cursiv::LspServicesFor<WktCoreServices, WktSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace wkt
