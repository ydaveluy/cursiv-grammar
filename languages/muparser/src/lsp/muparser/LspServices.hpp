// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "muparser/CoreServices.hpp"

namespace muparser {

struct MuparserSharedServices : MuparserSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MuparserServices final
    : cursiv::LspServicesFor<MuparserCoreServices, MuparserSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace muparser
