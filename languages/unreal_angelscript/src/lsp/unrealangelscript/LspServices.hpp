// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "unrealangelscript/CoreServices.hpp"

namespace unrealangelscript {

struct UnrealAngelscriptSharedServices : UnrealAngelscriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct UnrealAngelscriptServices final
    : cursiv::LspServicesFor<UnrealAngelscriptCoreServices, UnrealAngelscriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace unrealangelscript
