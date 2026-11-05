// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pmmn/CoreServices.hpp"

namespace pmmn {

struct PmmnSharedServices : PmmnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PmmnServices final
    : cursiv::LspServicesFor<PmmnCoreServices, PmmnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pmmn
