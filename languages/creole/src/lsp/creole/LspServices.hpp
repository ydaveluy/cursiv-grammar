// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "creole/CoreServices.hpp"

namespace creole {

struct CreoleSharedServices : CreoleSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CreoleServices final
    : cursiv::LspServicesFor<CreoleCoreServices, CreoleSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace creole
