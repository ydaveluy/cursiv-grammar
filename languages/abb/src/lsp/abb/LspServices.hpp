// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "abb/CoreServices.hpp"

namespace abb {

struct AbbSharedServices : AbbSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AbbServices final
    : cursiv::LspServicesFor<AbbCoreServices, AbbSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace abb
