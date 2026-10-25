// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "vb6/core/CoreServices.hpp"

namespace vb6 {

struct Vb6SharedServices : Vb6SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Vb6Services final : Vb6CoreServices,
                                           cursiv::LspServices {
  explicit Vb6Services(
      Vb6SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Vb6CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Vb6CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace vb6
