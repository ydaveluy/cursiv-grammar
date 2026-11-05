// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "cql3/CoreServices.hpp"

namespace cql3 {

struct Cql3SharedServices : Cql3SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Cql3Services final
    : cursiv::LspServicesFor<Cql3CoreServices, Cql3SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace cql3
