// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "rfc3080/core/CoreServices.hpp"

namespace rfc3080 {

struct Rfc3080SharedServices : Rfc3080SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc3080Services final : Rfc3080CoreServices,
                                           cursiv::LspServices {
  explicit Rfc3080Services(
      Rfc3080SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Rfc3080CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Rfc3080CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace rfc3080
