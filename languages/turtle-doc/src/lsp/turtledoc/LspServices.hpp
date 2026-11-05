// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "turtledoc/CoreServices.hpp"

namespace turtledoc {

struct TurtleDocSharedServices : TurtleDocSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TurtleDocServices final
    : cursiv::LspServicesFor<TurtleDocCoreServices, TurtleDocSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace turtledoc
