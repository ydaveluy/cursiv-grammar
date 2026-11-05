// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "karel/CoreServices.hpp"

namespace karel {

struct KarelSharedServices : KarelSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct KarelServices final
    : cursiv::LspServicesFor<KarelCoreServices, KarelSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace karel
