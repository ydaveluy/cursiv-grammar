// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "lambda/core/CoreServices.hpp"

namespace lambda {

struct LambdaSharedServices : LambdaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LambdaServices final : LambdaCoreServices,
                                           cursiv::LspServices {
  explicit LambdaServices(
      LambdaSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        LambdaCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    LambdaCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace lambda
