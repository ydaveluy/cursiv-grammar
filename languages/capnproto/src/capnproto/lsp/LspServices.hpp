// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "capnproto/core/CoreServices.hpp"

namespace capnproto {

struct CapnprotoSharedServices : CapnprotoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CapnprotoServices final : CapnprotoCoreServices,
                                           cursiv::LspServices {
  explicit CapnprotoServices(
      CapnprotoSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        CapnprotoCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}

  // One emission per storey through the diamond: the language rows through the
  // core side, the LSP rows alone.
  void visitServices(
      cursiv::function_ref<void(const cursiv::ServiceSlot&)> yield) const override {
    CapnprotoCoreServices::visitServices(yield);
    cursiv::LspServices::visitOwnServices(yield);
  }
};

}  // namespace capnproto
