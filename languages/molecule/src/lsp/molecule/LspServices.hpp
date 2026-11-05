// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "molecule/CoreServices.hpp"

namespace molecule {

struct MoleculeSharedServices : MoleculeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MoleculeServices final
    : cursiv::LspServicesFor<MoleculeCoreServices, MoleculeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace molecule
