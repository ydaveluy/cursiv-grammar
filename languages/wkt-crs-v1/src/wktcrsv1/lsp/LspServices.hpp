// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "wktcrsv1/core/CoreServices.hpp"

namespace wktcrsv1 {

struct WktCrsV1SharedServices : WktCrsV1SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct WktCrsV1Services final : WktCrsV1CoreServices,
                                           cursiv::LspServices {
  explicit WktCrsV1Services(
      WktCrsV1SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        WktCrsV1CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    WktCrsV1CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace wktcrsv1
