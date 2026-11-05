// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "bibcode/CoreServices.hpp"

namespace bibcode {

struct BibcodeSharedServices : BibcodeSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct BibcodeServices final
    : cursiv::LspServicesFor<BibcodeCoreServices, BibcodeSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace bibcode
