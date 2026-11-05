// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "rfc822/CoreServices.hpp"

namespace rfc822 {

struct Rfc822SharedServices : Rfc822SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc822Services final
    : cursiv::LspServicesFor<Rfc822CoreServices, Rfc822SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace rfc822
