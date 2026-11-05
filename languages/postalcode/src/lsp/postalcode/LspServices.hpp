// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "postalcode/CoreServices.hpp"

namespace postalcode {

struct PostalcodeSharedServices : PostalcodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct PostalcodeServices final
    : cursiv::LspServicesFor<PostalcodeCoreServices, PostalcodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace postalcode
