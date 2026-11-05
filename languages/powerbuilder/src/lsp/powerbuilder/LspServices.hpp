// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "powerbuilder/CoreServices.hpp"

namespace powerbuilder {

struct PowerbuilderSharedServices : PowerbuilderSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PowerbuilderServices final
    : cursiv::LspServicesFor<PowerbuilderCoreServices, PowerbuilderSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace powerbuilder
