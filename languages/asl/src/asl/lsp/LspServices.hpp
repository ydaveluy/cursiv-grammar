// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "asl/core/CoreServices.hpp"

namespace asl {

struct AslSharedServices : AslSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AslServices final : AslCoreServices,
                                           cursiv::LspServices {
  explicit AslServices(
      AslSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        AslCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    AslCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace asl
