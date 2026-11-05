// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "ebnf/CoreServices.hpp"

namespace ebnf {

struct EbnfSharedServices : EbnfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct EbnfServices final
    : cursiv::LspServicesFor<EbnfCoreServices, EbnfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace ebnf
