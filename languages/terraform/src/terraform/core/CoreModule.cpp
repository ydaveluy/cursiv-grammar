// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "terraform/core/CoreServices.hpp"
#include "terraform/core/validation/TerraformValidator.hpp"
#include "terraform/generated/Terraform/Module.hpp"

namespace terraform {

void install_core_module(TerraformCoreServices& services) {
  terraform::generated::Terraform::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace terraform
