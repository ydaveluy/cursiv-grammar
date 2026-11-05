// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "html/CoreServices.hpp"

namespace html {

struct HtmlSharedServices : HtmlSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct HtmlServices final
    : cursiv::LspServicesFor<HtmlCoreServices, HtmlSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace html
