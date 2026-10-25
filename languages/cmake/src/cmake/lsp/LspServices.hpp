// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "cmake/core/CoreServices.hpp"

namespace cmake {

struct CmakeSharedServices : CmakeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CmakeServices final : CmakeCoreServices,
                                           cursiv::LspServices {
  explicit CmakeServices(
      CmakeSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        CmakeCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    CmakeCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace cmake
