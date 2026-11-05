// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "stringtemplate/CoreServices.hpp"

namespace stringtemplate {

struct StringtemplateSharedServices : StringtemplateSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct StringtemplateServices final
    : cursiv::LspServicesFor<StringtemplateCoreServices, StringtemplateSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace stringtemplate
