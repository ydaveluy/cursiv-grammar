// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "c/CoreServices.hpp"

namespace c {

struct CSharedServices : CSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CServices final
    : cursiv::LspServicesFor<CCoreServices, CSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace c
