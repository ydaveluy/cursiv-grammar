// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "lucene/CoreServices.hpp"

namespace lucene {

struct LuceneSharedServices : LuceneSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct LuceneServices final
    : cursiv::LspServicesFor<LuceneCoreServices, LuceneSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace lucene
