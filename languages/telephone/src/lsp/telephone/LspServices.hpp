// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "telephone/CoreServices.hpp"

namespace telephone {

struct TelephoneSharedServices : TelephoneSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TelephoneServices final
    : cursiv::LspServicesFor<TelephoneCoreServices, TelephoneSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace telephone
