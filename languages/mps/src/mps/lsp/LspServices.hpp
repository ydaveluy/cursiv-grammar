// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "mps/core/CoreServices.hpp"

namespace mps {

struct MpsSharedServices : MpsSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MpsServices final : MpsCoreServices,
                                           cursiv::LspServices {
  explicit MpsServices(
      MpsSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        MpsCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    MpsCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace mps
