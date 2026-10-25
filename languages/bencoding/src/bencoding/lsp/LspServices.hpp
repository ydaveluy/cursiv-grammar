// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "bencoding/core/CoreServices.hpp"

namespace bencoding {

struct BencodingSharedServices : BencodingSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BencodingServices final : BencodingCoreServices,
                                           cursiv::LspServices {
  explicit BencodingServices(
      BencodingSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        BencodingCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    BencodingCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace bencoding
