// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "smiles/CoreServices.hpp"

namespace smiles {

struct SmilesSharedServices : SmilesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SmilesServices final
    : cursiv::LspServicesFor<SmilesCoreServices, SmilesSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace smiles
