// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "teal/CoreServices.hpp"

namespace teal {

struct TealSharedServices : TealSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TealServices final
    : cursiv::LspServicesFor<TealCoreServices, TealSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace teal
