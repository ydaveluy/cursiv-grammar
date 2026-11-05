// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "velocity/CoreServices.hpp"

namespace velocity {

struct VelocitySharedServices : VelocitySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct VelocityServices final
    : cursiv::LspServicesFor<VelocityCoreServices, VelocitySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace velocity
