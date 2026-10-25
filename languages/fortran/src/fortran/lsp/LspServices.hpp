// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "fortran/core/CoreServices.hpp"

namespace fortran {

struct FortranSharedServices : FortranSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FortranServices final : FortranCoreServices,
                                           cursiv::LspServices {
  explicit FortranServices(
      FortranSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        FortranCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    FortranCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace fortran
