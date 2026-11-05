// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "morsecode/CoreServices.hpp"

namespace morsecode {

struct MorsecodeSharedServices : MorsecodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MorsecodeServices final
    : cursiv::LspServicesFor<MorsecodeCoreServices, MorsecodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace morsecode
