// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bcl/CoreServices.hpp"

namespace bcl {

struct BclSharedServices : BclSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BclServices final
    : cursiv::LspServicesFor<BclCoreServices, BclSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bcl
