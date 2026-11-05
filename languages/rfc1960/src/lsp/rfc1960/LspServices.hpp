// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rfc1960/CoreServices.hpp"

namespace rfc1960 {

struct Rfc1960SharedServices : Rfc1960SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc1960Services final
    : cursiv::LspServicesFor<Rfc1960CoreServices, Rfc1960SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rfc1960
