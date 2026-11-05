// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "robotwars/CoreServices.hpp"

namespace robotwars {

struct RobotwarsSharedServices : RobotwarsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RobotwarsServices final
    : cursiv::LspServicesFor<RobotwarsCoreServices, RobotwarsSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace robotwars
