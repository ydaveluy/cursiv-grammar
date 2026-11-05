// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "sexpression/CoreServices.hpp"

namespace sexpression {

struct SexpressionSharedServices : SexpressionSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct SexpressionServices final
    : cursiv::LspServicesFor<SexpressionCoreServices, SexpressionSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace sexpression
