// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "racketisl/CoreServices.hpp"

namespace racketisl {

struct RacketIslSharedServices : RacketIslSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RacketIslServices final
    : cursiv::LspServicesFor<RacketIslCoreServices, RacketIslSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace racketisl
