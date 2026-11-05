// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sapf/CoreServices.hpp"

namespace sapf {

struct SapfSharedServices : SapfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SapfServices final
    : cursiv::LspServicesFor<SapfCoreServices, SapfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sapf
