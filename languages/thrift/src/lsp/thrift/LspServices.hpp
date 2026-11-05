// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "thrift/CoreServices.hpp"

namespace thrift {

struct ThriftSharedServices : ThriftSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ThriftServices final
    : cursiv::LspServicesFor<ThriftCoreServices, ThriftSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace thrift
