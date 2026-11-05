// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "mdx/CoreServices.hpp"

namespace mdx {

struct MdxSharedServices : MdxSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct MdxServices final
    : cursiv::LspServicesFor<MdxCoreServices, MdxSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace mdx
