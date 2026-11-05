// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "scss/CoreServices.hpp"

namespace scss {

struct ScssSharedServices : ScssSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ScssServices final
    : cursiv::LspServicesFor<ScssCoreServices, ScssSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace scss
