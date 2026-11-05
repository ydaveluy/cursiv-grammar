// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "databank/CoreServices.hpp"

namespace databank {

struct DatabankSharedServices : DatabankSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DatabankServices final
    : cursiv::LspServicesFor<DatabankCoreServices, DatabankSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace databank
