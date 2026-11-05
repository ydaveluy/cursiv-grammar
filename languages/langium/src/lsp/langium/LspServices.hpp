// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "langium/CoreServices.hpp"

namespace langium {

struct LangiumSharedServices : LangiumSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LangiumServices final
    : cursiv::LspServicesFor<LangiumCoreServices, LangiumSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace langium
