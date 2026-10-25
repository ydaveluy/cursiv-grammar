// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "rfc1960/core/CoreServices.hpp"

namespace rfc1960 {

struct Rfc1960SharedServices : Rfc1960SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Rfc1960Services final : Rfc1960CoreServices,
                                           cursiv::LspServices {
  explicit Rfc1960Services(
      Rfc1960SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Rfc1960CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Rfc1960CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace rfc1960
