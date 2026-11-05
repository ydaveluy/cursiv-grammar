// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bnf/CoreServices.hpp"

namespace bnf {

struct BnfSharedServices : BnfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BnfServices final
    : cursiv::LspServicesFor<BnfCoreServices, BnfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bnf
