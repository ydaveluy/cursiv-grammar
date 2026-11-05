// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ruleworks/CoreServices.hpp"

namespace ruleworks {

struct RuleworksSharedServices : RuleworksSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RuleworksServices final
    : cursiv::LspServicesFor<RuleworksCoreServices, RuleworksSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ruleworks
