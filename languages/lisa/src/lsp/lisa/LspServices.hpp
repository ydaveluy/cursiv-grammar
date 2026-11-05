// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lisa/CoreServices.hpp"

namespace lisa {

struct LisaSharedServices : LisaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LisaServices final
    : cursiv::LspServicesFor<LisaCoreServices, LisaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lisa
