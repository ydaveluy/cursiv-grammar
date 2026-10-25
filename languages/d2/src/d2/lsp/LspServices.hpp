// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "d2/core/CoreServices.hpp"

namespace d2 {

struct D2SharedServices : D2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct D2Services final : D2CoreServices,
                                           cursiv::LspServices {
  explicit D2Services(
      D2SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        D2CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    D2CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace d2
