// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "edif300/core/CoreServices.hpp"

namespace edif300 {

struct Edif300SharedServices : Edif300SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Edif300Services final : Edif300CoreServices,
                                           cursiv::LspServices {
  explicit Edif300Services(
      Edif300SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Edif300CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Edif300CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace edif300
