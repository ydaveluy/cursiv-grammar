// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "romannumerals/CoreServices.hpp"

namespace romannumerals {

struct RomannumeralsSharedServices : RomannumeralsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RomannumeralsServices final
    : cursiv::LspServicesFor<RomannumeralsCoreServices, RomannumeralsSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace romannumerals
