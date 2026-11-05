// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fen/CoreServices.hpp"

namespace fen {

struct FenSharedServices : FenSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FenServices final
    : cursiv::LspServicesFor<FenCoreServices, FenSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fen
