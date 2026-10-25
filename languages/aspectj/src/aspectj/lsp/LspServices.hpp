// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "aspectj/core/CoreServices.hpp"

namespace aspectj {

struct AspectjSharedServices : AspectjSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AspectjServices final : AspectjCoreServices,
                                           cursiv::LspServices {
  explicit AspectjServices(
      AspectjSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        AspectjCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    AspectjCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace aspectj
