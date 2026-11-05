// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "memcachedprotocol/CoreServices.hpp"

namespace memcachedprotocol {

struct MemcachedProtocolSharedServices : MemcachedProtocolSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MemcachedProtocolServices final
    : cursiv::LspServicesFor<MemcachedProtocolCoreServices, MemcachedProtocolSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace memcachedprotocol
