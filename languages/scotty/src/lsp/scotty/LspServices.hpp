// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "scotty/CoreServices.hpp"

namespace scotty {

struct ScottySharedServices : ScottySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ScottyServices final
    : cursiv::LspServicesFor<ScottyCoreServices, ScottySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace scotty
