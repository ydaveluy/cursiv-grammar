// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "protobuf/CoreServices.hpp"

namespace protobuf {

struct ProtobufSharedServices : ProtobufSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ProtobufServices final
    : cursiv::LspServicesFor<ProtobufCoreServices, ProtobufSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace protobuf
