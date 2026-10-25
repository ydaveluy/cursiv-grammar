// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "cobol85/core/CoreServices.hpp"

namespace cobol85 {

struct Cobol85SharedServices : Cobol85SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Cobol85Services final : Cobol85CoreServices,
                                           cursiv::LspServices {
  explicit Cobol85Services(
      Cobol85SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Cobol85CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Cobol85CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace cobol85
