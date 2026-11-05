// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "abnf/CoreServices.hpp"

namespace abnf {

struct AbnfSharedServices : AbnfSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AbnfServices final
    : cursiv::LspServicesFor<AbnfCoreServices, AbnfSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace abnf
