// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "flowmatic/CoreServices.hpp"

namespace flowmatic {

struct FlowmaticSharedServices : FlowmaticSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FlowmaticServices final
    : cursiv::LspServicesFor<FlowmaticCoreServices, FlowmaticSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace flowmatic
