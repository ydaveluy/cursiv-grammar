// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tnt/CoreServices.hpp"

namespace tnt {

struct TntSharedServices : TntSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TntServices final
    : cursiv::LspServicesFor<TntCoreServices, TntSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tnt
