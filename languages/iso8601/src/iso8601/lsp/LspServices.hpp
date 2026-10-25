// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "iso8601/core/CoreServices.hpp"

namespace iso8601 {

struct Iso8601SharedServices : Iso8601SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Iso8601Services final : Iso8601CoreServices,
                                           cursiv::LspServices {
  explicit Iso8601Services(
      Iso8601SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Iso8601CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Iso8601CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace iso8601
