// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "tcpheader/CoreServices.hpp"

namespace tcpheader {

struct TcpheaderSharedServices : TcpheaderSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TcpheaderServices final
    : cursiv::LspServicesFor<TcpheaderCoreServices, TcpheaderSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace tcpheader
