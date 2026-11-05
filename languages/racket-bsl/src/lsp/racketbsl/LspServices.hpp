// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "racketbsl/CoreServices.hpp"

namespace racketbsl {

struct RacketBslSharedServices : RacketBslSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RacketBslServices final
    : cursiv::LspServicesFor<RacketBslCoreServices, RacketBslSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace racketbsl
