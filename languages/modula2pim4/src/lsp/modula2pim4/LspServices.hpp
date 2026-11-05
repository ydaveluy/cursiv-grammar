// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "modula2pim4/CoreServices.hpp"

namespace modula2pim4 {

struct Modula2pim4SharedServices : Modula2pim4SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Modula2pim4Services final
    : cursiv::LspServicesFor<Modula2pim4CoreServices, Modula2pim4SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace modula2pim4
