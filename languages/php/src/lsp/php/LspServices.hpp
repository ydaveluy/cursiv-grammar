// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "php/CoreServices.hpp"

namespace php {

struct PhpSharedServices : PhpSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PhpServices final
    : cursiv::LspServicesFor<PhpCoreServices, PhpSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace php
