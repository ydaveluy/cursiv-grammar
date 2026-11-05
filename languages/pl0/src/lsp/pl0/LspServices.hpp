// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "pl0/CoreServices.hpp"

namespace pl0 {

struct Pl0SharedServices : Pl0SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Pl0Services final
    : cursiv::LspServicesFor<Pl0CoreServices, Pl0SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace pl0
