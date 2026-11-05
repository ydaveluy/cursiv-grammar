// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "plucid/CoreServices.hpp"

namespace plucid {

struct PlucidSharedServices : PlucidSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PlucidServices final
    : cursiv::LspServicesFor<PlucidCoreServices, PlucidSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace plucid
