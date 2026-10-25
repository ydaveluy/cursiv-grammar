// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "pl0/core/CoreServices.hpp"

namespace pl0 {

struct Pl0SharedServices : Pl0SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Pl0Services final : Pl0CoreServices,
                                           cursiv::LspServices {
  explicit Pl0Services(
      Pl0SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Pl0CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Pl0CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace pl0
