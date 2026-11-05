// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "alloy/CoreServices.hpp"

namespace alloy {

struct AlloySharedServices : AlloySharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AlloyServices final
    : cursiv::LspServicesFor<AlloyCoreServices, AlloySharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace alloy
