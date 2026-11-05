// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "xsdregex/CoreServices.hpp"

namespace xsdregex {

struct XsdRegexSharedServices : XsdRegexSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct XsdRegexServices final
    : cursiv::LspServicesFor<XsdRegexCoreServices, XsdRegexSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace xsdregex
