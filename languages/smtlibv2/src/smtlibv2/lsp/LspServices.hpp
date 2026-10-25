// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "smtlibv2/core/CoreServices.hpp"

namespace smtlibv2 {

struct Smtlibv2SharedServices : Smtlibv2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Smtlibv2Services final : Smtlibv2CoreServices,
                                           cursiv::LspServices {
  explicit Smtlibv2Services(
      Smtlibv2SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Smtlibv2CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Smtlibv2CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace smtlibv2
