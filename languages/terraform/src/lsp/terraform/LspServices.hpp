// The same language one storey up. Both containers derive `CoreServices`
// virtually, so this holds one of it and the core module wires the same
// object the LSP providers read.
#pragma once

#include "cursiv/services/LspServicesFor.hpp"
#include "cursiv/services/SharedLspServices.hpp"
#include "terraform/CoreServices.hpp"

namespace terraform {

struct TerraformSharedServices : TerraformSharedCoreServices,
                                           cursiv::SharedLspServices {};

struct TerraformServices final
    : cursiv::LspServicesFor<TerraformCoreServices, TerraformSharedServices> {
  using LspServicesFor::LspServicesFor;
};

}  // namespace terraform
