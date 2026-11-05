// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "z/CoreServices.hpp"

namespace z {

struct ZSharedServices : ZSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ZServices final
    : cursiv::LspServicesFor<ZCoreServices, ZSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace z
