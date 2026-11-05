// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bencoding/CoreServices.hpp"

namespace bencoding {

struct BencodingSharedServices : BencodingSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BencodingServices final
    : cursiv::LspServicesFor<BencodingCoreServices, BencodingSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bencoding
