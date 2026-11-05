// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pest/CoreServices.hpp"

namespace pest {

struct PestSharedServices : PestSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PestServices final
    : cursiv::LspServicesFor<PestCoreServices, PestSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pest
