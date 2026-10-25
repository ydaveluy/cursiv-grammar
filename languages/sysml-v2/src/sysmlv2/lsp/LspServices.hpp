// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "sysmlv2/core/CoreServices.hpp"

namespace sysmlv2 {

struct SysmlV2SharedServices : SysmlV2SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SysmlV2Services final : SysmlV2CoreServices,
                                           cursiv::LspServices {
  explicit SysmlV2Services(
      SysmlV2SharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        SysmlV2CoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    SysmlV2CoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace sysmlv2
