// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "css3/core/CoreServices.hpp"

namespace css3 {

struct Css3SharedServices : Css3SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Css3Services final : Css3CoreServices,
                                           cursiv::LspServices {
  explicit Css3Services(
      Css3SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Css3CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Css3CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace css3
