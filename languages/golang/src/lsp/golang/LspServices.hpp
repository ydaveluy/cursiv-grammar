// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "golang/CoreServices.hpp"

namespace golang {

struct GolangSharedServices : GolangSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GolangServices final
    : cursiv::LspServicesFor<GolangCoreServices, GolangSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace golang
