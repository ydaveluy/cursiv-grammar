// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "chip8/CoreServices.hpp"

namespace chip8 {

struct Chip8SharedServices : Chip8SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Chip8Services final
    : cursiv::LspServicesFor<Chip8CoreServices, Chip8SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace chip8
