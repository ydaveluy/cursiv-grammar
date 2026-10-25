// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "fdo91/core/CoreServices.hpp"

namespace fdo91 {

struct Fdo91SharedServices : Fdo91SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Fdo91Services final : Fdo91CoreServices,
                                           cursiv::LspServices {
  explicit Fdo91Services(
      Fdo91SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        Fdo91CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    Fdo91CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace fdo91
