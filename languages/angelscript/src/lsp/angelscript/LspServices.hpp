// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "angelscript/CoreServices.hpp"

namespace angelscript {

struct AngelscriptSharedServices : AngelscriptSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AngelscriptServices final
    : cursiv::LspServicesFor<AngelscriptCoreServices, AngelscriptSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace angelscript
