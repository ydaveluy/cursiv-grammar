// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "wln/CoreServices.hpp"

namespace wln {

struct WlnSharedServices : WlnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WlnServices final
    : cursiv::LspServicesFor<WlnCoreServices, WlnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace wln
