// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "action/core/CoreServices.hpp"

namespace action {

struct ActionSharedServices : ActionSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ActionServices final : ActionCoreServices,
                                           cursiv::LspServices {
  explicit ActionServices(
      ActionSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        ActionCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    ActionCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace action
