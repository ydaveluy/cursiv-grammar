// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "vb6/CoreServices.hpp"

namespace vb6 {

struct Vb6SharedServices : Vb6SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Vb6Services final
    : cursiv::LspServicesFor<Vb6CoreServices, Vb6SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace vb6
