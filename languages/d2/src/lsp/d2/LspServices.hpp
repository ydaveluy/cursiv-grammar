// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "d2/CoreServices.hpp"

namespace d2 {

struct D2SharedServices : D2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct D2Services final
    : cursiv::LspServicesFor<D2CoreServices, D2SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace d2
