// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "algol60/core/CoreServices.hpp"

namespace algol60 {

struct Algol60SharedServices : Algol60SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Algol60Services final : Algol60CoreServices,
                                           cursiv::LspServices {
  explicit Algol60Services(
      Algol60SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Algol60CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Algol60CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace algol60
