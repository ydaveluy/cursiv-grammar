// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "dart2/core/CoreServices.hpp"

namespace dart2 {

struct Dart2SharedServices : Dart2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Dart2Services final : Dart2CoreServices,
                                           cursiv::LspServices {
  explicit Dart2Services(
      Dart2SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Dart2CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Dart2CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace dart2
