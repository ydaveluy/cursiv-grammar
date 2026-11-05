// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "geekcode/CoreServices.hpp"

namespace geekcode {

struct GeekcodeSharedServices : GeekcodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct GeekcodeServices final
    : cursiv::LspServicesFor<GeekcodeCoreServices, GeekcodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace geekcode
