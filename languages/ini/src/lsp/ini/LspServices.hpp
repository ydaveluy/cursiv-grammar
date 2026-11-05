#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ini/CoreServices.hpp"

// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.

namespace ini {

struct IniSharedServices : IniSharedCoreServices, cursiv::SharedLspServices {};

struct IniServices final
    : cursiv::LspServicesFor<IniCoreServices, IniSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ini
