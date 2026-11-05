// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "restructuredtext/CoreServices.hpp"

namespace restructuredtext {

struct RestructuredtextSharedServices : RestructuredtextSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct RestructuredtextServices final
    : cursiv::LspServicesFor<RestructuredtextCoreServices, RestructuredtextSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace restructuredtext
