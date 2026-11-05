// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "argus/CoreServices.hpp"

namespace argus {

struct ArgusSharedServices : ArgusSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ArgusServices final
    : cursiv::LspServicesFor<ArgusCoreServices, ArgusSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace argus
