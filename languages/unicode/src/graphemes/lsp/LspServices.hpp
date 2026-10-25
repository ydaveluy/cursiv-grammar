// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "graphemes/core/CoreServices.hpp"

namespace graphemes {

struct GraphemesSharedServices : GraphemesSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GraphemesServices final : GraphemesCoreServices,
                                           cursiv::LspServices {
  explicit GraphemesServices(
      GraphemesSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        GraphemesCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    GraphemesCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace graphemes
