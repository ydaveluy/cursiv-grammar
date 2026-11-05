// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rpn/CoreServices.hpp"

namespace rpn {

struct RpnSharedServices : RpnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RpnServices final
    : cursiv::LspServicesFor<RpnCoreServices, RpnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rpn
