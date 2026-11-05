// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rfc1035/CoreServices.hpp"

namespace rfc1035 {

struct Rfc1035SharedServices : Rfc1035SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc1035Services final
    : cursiv::LspServicesFor<Rfc1035CoreServices, Rfc1035SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rfc1035
