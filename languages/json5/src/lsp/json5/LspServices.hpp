// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "json5/CoreServices.hpp"

namespace json5 {

struct Json5SharedServices : Json5SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Json5Services final
    : cursiv::LspServicesFor<Json5CoreServices, Json5SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace json5
