// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "snobol/CoreServices.hpp"

namespace snobol {

struct SnobolSharedServices : SnobolSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SnobolServices final
    : cursiv::LspServicesFor<SnobolCoreServices, SnobolSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace snobol
