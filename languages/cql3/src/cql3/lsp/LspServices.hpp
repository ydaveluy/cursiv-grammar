// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "cql3/core/CoreServices.hpp"

namespace cql3 {

struct Cql3SharedServices : Cql3SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Cql3Services final : Cql3CoreServices,
                                           cursiv::LspServices {
  explicit Cql3Services(
      Cql3SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Cql3CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Cql3CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace cql3
