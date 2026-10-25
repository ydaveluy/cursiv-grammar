// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "c/core/CoreServices.hpp"

namespace c {

struct CSharedServices : CSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CServices final : CCoreServices,
                                           cursiv::LspServices {
  explicit CServices(
      CSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        CCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    CCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace c
