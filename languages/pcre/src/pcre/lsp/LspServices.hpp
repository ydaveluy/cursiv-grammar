// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "pcre/core/CoreServices.hpp"

namespace pcre {

struct PcreSharedServices : PcreSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PcreServices final : PcreCoreServices,
                                           cursiv::LspServices {
  explicit PcreServices(
      PcreSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        PcreCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    PcreCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace pcre
