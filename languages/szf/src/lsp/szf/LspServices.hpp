// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "szf/CoreServices.hpp"

namespace szf {

struct SzfSharedServices : SzfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SzfServices final
    : cursiv::LspServicesFor<SzfCoreServices, SzfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace szf
