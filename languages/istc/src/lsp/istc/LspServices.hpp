// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "istc/CoreServices.hpp"

namespace istc {

struct IstcSharedServices : IstcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct IstcServices final
    : cursiv::LspServicesFor<IstcCoreServices, IstcSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace istc
