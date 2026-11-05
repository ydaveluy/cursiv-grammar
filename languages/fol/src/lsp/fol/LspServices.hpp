// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fol/CoreServices.hpp"

namespace fol {

struct FolSharedServices : FolSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FolServices final
    : cursiv::LspServicesFor<FolCoreServices, FolSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fol
