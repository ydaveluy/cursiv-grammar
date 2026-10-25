// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "smiles/core/CoreServices.hpp"

namespace smiles {

struct SmilesSharedServices : SmilesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SmilesServices final : SmilesCoreServices,
                                           cursiv::LspServices {
  explicit SmilesServices(
      SmilesSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        SmilesCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    SmilesCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace smiles
