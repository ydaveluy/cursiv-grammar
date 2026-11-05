// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "dice/CoreServices.hpp"

namespace dice {

struct DiceSharedServices : DiceSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct DiceServices final
    : cursiv::LspServicesFor<DiceCoreServices, DiceSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace dice
