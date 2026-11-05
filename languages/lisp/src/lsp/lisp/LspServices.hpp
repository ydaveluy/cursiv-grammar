// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lisp/CoreServices.hpp"

namespace lisp {

struct LispSharedServices : LispSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LispServices final
    : cursiv::LspServicesFor<LispCoreServices, LispSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lisp
