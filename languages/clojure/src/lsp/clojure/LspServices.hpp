// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "clojure/CoreServices.hpp"

namespace clojure {

struct ClojureSharedServices : ClojureSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct ClojureServices final
    : cursiv::LspServicesFor<ClojureCoreServices, ClojureSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace clojure
