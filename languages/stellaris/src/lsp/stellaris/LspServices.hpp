// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "stellaris/CoreServices.hpp"

namespace stellaris {

struct StellarisSharedServices : StellarisSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct StellarisServices final
    : cursiv::LspServicesFor<StellarisCoreServices, StellarisSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace stellaris
