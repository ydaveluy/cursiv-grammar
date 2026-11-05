// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "smalltalk/CoreServices.hpp"

namespace smalltalk {

struct SmalltalkSharedServices : SmalltalkSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SmalltalkServices final
    : cursiv::LspServicesFor<SmalltalkCoreServices, SmalltalkSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace smalltalk
