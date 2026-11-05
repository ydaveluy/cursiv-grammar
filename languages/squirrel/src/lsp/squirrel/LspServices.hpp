// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "squirrel/CoreServices.hpp"

namespace squirrel {

struct SquirrelSharedServices : SquirrelSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SquirrelServices final
    : cursiv::LspServicesFor<SquirrelCoreServices, SquirrelSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace squirrel
