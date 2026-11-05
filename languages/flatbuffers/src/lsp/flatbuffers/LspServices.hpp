// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "flatbuffers/CoreServices.hpp"

namespace flatbuffers {

struct FlatbuffersSharedServices : FlatbuffersSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FlatbuffersServices final
    : cursiv::LspServicesFor<FlatbuffersCoreServices, FlatbuffersSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace flatbuffers
