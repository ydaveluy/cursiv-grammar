// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "objc/core/CoreServices.hpp"

namespace objc {

struct ObjcSharedServices : ObjcSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ObjcServices final : ObjcCoreServices,
                                           cursiv::LspServices {
  explicit ObjcServices(
      ObjcSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        ObjcCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    ObjcCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace objc
