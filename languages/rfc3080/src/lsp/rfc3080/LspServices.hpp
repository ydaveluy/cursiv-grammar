// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rfc3080/CoreServices.hpp"

namespace rfc3080 {

struct Rfc3080SharedServices : Rfc3080SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc3080Services final
    : cursiv::LspServicesFor<Rfc3080CoreServices, Rfc3080SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rfc3080
