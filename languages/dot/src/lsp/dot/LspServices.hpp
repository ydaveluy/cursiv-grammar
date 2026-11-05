// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "dot/CoreServices.hpp"

namespace dot {

struct DotSharedServices : DotSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DotServices final
    : cursiv::LspServicesFor<DotCoreServices, DotSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace dot
