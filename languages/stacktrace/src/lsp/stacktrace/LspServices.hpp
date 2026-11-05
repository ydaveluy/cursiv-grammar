// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "stacktrace/CoreServices.hpp"

namespace stacktrace {

struct StacktraceSharedServices : StacktraceSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct StacktraceServices final
    : cursiv::LspServicesFor<StacktraceCoreServices, StacktraceSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace stacktrace
