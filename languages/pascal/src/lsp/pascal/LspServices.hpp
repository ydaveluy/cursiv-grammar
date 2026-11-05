// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pascal/CoreServices.hpp"

namespace pascal {

struct PascalSharedServices : PascalSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PascalServices final
    : cursiv::LspServicesFor<PascalCoreServices, PascalSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pascal
