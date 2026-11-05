// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "suokif/CoreServices.hpp"

namespace suokif {

struct SuokifSharedServices : SuokifSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SuokifServices final
    : cursiv::LspServicesFor<SuokifCoreServices, SuokifSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace suokif
