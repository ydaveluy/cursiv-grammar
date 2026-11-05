// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "aspectj/CoreServices.hpp"

namespace aspectj {

struct AspectjSharedServices : AspectjSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AspectjServices final
    : cursiv::LspServicesFor<AspectjCoreServices, AspectjSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace aspectj
