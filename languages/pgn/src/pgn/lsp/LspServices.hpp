// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "pgn/core/CoreServices.hpp"

namespace pgn {

struct PgnSharedServices : PgnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PgnServices final : PgnCoreServices,
                                           cursiv::LspServices {
  explicit PgnServices(
      PgnSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        PgnCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    PgnCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace pgn
