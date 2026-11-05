// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "miniscript/CoreServices.hpp"

namespace miniscript {

struct MiniscriptSharedServices : MiniscriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MiniscriptServices final
    : cursiv::LspServicesFor<MiniscriptCoreServices, MiniscriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace miniscript
