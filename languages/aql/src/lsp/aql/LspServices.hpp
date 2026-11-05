// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "aql/CoreServices.hpp"

namespace aql {

struct AqlSharedServices : AqlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AqlServices final
    : cursiv::LspServicesFor<AqlCoreServices, AqlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace aql
