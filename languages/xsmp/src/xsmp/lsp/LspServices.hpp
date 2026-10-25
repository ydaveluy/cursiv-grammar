#pragma once

#include "cursiv/lsp/services/LspServices.hpp"
#include "cursiv/lsp/services/SharedLspServices.hpp"
#include "xsmp/core/CoreServices.hpp"

// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.

namespace xsmp {

struct XsmpSharedServices : XsmpSharedCoreServices,
                            cursiv::SharedLspServices {};

struct XsmpServices final : XsmpCoreServices, cursiv::LspServices {
  explicit XsmpServices(XsmpSharedServices& sharedServices)
      : cursiv::CoreServices(sharedServices),
        XsmpCoreServices(sharedServices),
        cursiv::LspServices(sharedServices) {}
};

}  // namespace xsmp
