// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "prolog/CoreServices.hpp"

namespace prolog {

struct PrologSharedServices : PrologSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PrologServices final
    : cursiv::LspServicesFor<PrologCoreServices, PrologSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace prolog
