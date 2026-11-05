// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "p/CoreServices.hpp"

namespace p {

struct PSharedServices : PSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PServices final
    : cursiv::LspServicesFor<PCoreServices, PSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace p
