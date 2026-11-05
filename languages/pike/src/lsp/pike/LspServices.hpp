// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pike/CoreServices.hpp"

namespace pike {

struct PikeSharedServices : PikeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PikeServices final
    : cursiv::LspServicesFor<PikeCoreServices, PikeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pike
