// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "chip8/core/CoreServices.hpp"

namespace chip8 {

struct Chip8SharedServices : Chip8SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Chip8Services final : Chip8CoreServices,
                                           cursiv::LspServices {
  explicit Chip8Services(
      Chip8SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Chip8CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Chip8CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace chip8
