// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "xml/CoreServices.hpp"

namespace xml {

struct XmlSharedServices : XmlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct XmlServices final
    : cursiv::LspServicesFor<XmlCoreServices, XmlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace xml
