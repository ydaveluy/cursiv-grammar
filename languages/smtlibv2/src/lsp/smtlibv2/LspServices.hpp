// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "smtlibv2/CoreServices.hpp"

namespace smtlibv2 {

struct Smtlibv2SharedServices : Smtlibv2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Smtlibv2Services final
    : cursiv::LspServicesFor<Smtlibv2CoreServices, Smtlibv2SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace smtlibv2
