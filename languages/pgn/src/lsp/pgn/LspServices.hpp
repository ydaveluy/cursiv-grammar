// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pgn/CoreServices.hpp"

namespace pgn {

struct PgnSharedServices : PgnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PgnServices final
    : cursiv::LspServicesFor<PgnCoreServices, PgnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pgn
