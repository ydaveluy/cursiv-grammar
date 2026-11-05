// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "fasta/CoreServices.hpp"

namespace fasta {

struct FastaSharedServices : FastaSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct FastaServices final
    : cursiv::LspServicesFor<FastaCoreServices, FastaSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace fasta
