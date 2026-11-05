// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "erlang/CoreServices.hpp"

namespace erlang {

struct ErlangSharedServices : ErlangSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ErlangServices final
    : cursiv::LspServicesFor<ErlangCoreServices, ErlangSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace erlang
