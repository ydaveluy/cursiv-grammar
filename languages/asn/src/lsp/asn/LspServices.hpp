// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "asn/CoreServices.hpp"

namespace asn {

struct AsnSharedServices : AsnSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct AsnServices final
    : cursiv::LspServicesFor<AsnCoreServices, AsnSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace asn
