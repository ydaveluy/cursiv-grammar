// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "json5/core/CoreServices.hpp"

namespace json5 {

struct Json5SharedServices : Json5SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Json5Services final : Json5CoreServices,
                                           cursiv::LspServices {
  explicit Json5Services(
      Json5SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Json5CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Json5CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace json5
