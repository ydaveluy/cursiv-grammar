// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "modula2pim4/core/CoreServices.hpp"

namespace modula2pim4 {

struct Modula2pim4SharedServices : Modula2pim4SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Modula2pim4Services final : Modula2pim4CoreServices,
                                           cursiv::LspServices {
  explicit Modula2pim4Services(
      Modula2pim4SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Modula2pim4CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Modula2pim4CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace modula2pim4
