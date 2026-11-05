// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "b/CoreServices.hpp"

namespace b {

struct BSharedServices : BSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BServices final
    : cursiv::LspServicesFor<BCoreServices, BSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace b
