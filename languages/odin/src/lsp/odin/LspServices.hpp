// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "odin/CoreServices.hpp"

namespace odin {

struct OdinSharedServices : OdinSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct OdinServices final
    : cursiv::LspServicesFor<OdinCoreServices, OdinSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace odin
