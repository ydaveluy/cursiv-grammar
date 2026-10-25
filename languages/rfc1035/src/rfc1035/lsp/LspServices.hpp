// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "rfc1035/core/CoreServices.hpp"

namespace rfc1035 {

struct Rfc1035SharedServices : Rfc1035SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc1035Services final : Rfc1035CoreServices,
                                           cursiv::LspServices {
  explicit Rfc1035Services(
      Rfc1035SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Rfc1035CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Rfc1035CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace rfc1035
