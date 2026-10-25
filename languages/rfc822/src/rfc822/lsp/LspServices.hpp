// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "rfc822/core/CoreServices.hpp"

namespace rfc822 {

struct Rfc822SharedServices : Rfc822SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc822Services final : Rfc822CoreServices,
                                           cursiv::LspServices {
  explicit Rfc822Services(
      Rfc822SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Rfc822CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Rfc822CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace rfc822
