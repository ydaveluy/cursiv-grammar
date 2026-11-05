// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "iso8601/CoreServices.hpp"

namespace iso8601 {

struct Iso8601SharedServices : Iso8601SharedCoreServices,
                                           cursiv::SharedLspServices {};

struct Iso8601Services final
    : cursiv::LspServicesFor<Iso8601CoreServices, Iso8601SharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace iso8601
