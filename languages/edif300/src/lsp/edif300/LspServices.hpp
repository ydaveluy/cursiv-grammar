// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "edif300/CoreServices.hpp"

namespace edif300 {

struct Edif300SharedServices : Edif300SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Edif300Services final
    : cursiv::LspServicesFor<Edif300CoreServices, Edif300SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace edif300
