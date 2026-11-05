// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "eiffel/CoreServices.hpp"

namespace eiffel {

struct EiffelSharedServices : EiffelSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct EiffelServices final
    : cursiv::LspServicesFor<EiffelCoreServices, EiffelSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace eiffel
