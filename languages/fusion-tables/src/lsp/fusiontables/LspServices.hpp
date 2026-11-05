// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fusiontables/CoreServices.hpp"

namespace fusiontables {

struct FusionTablesSharedServices : FusionTablesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FusionTablesServices final
    : cursiv::LspServicesFor<FusionTablesCoreServices, FusionTablesSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fusiontables
