// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "capnproto/CoreServices.hpp"

namespace capnproto {

struct CapnprotoSharedServices : CapnprotoSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct CapnprotoServices final
    : cursiv::LspServicesFor<CapnprotoCoreServices, CapnprotoSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace capnproto
