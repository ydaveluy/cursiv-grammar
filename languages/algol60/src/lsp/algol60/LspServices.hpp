// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "algol60/CoreServices.hpp"

namespace algol60 {

struct Algol60SharedServices : Algol60SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Algol60Services final
    : cursiv::LspServicesFor<Algol60CoreServices, Algol60SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace algol60
