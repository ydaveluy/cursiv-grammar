// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lolcode/CoreServices.hpp"

namespace lolcode {

struct LolcodeSharedServices : LolcodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LolcodeServices final
    : cursiv::LspServicesFor<LolcodeCoreServices, LolcodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lolcode
