// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "arden/CoreServices.hpp"

namespace arden {

struct ArdenSharedServices : ArdenSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ArdenServices final
    : cursiv::LspServicesFor<ArdenCoreServices, ArdenSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace arden
