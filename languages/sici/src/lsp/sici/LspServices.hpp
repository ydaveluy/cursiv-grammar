// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sici/CoreServices.hpp"

namespace sici {

struct SiciSharedServices : SiciSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SiciServices final
    : cursiv::LspServicesFor<SiciCoreServices, SiciSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sici
