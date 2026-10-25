// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "ocl/core/CoreServices.hpp"

namespace ocl {

struct OclSharedServices : OclSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct OclServices final : OclCoreServices,
                                           cursiv::LspServices {
  explicit OclServices(
      OclSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        OclCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    OclCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace ocl
