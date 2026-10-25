#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "ini/core/CoreServices.hpp"

// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.

namespace ini {

struct IniSharedServices : IniSharedCoreServices, cursiv::SharedLspServices {};

struct IniServices final : IniCoreServices, cursiv::LspServices {
  explicit IniServices(IniSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        IniCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    IniCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace ini
