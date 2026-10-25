// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "mdx/core/CoreServices.hpp"
#include "mdx/core/validation/MdxValidator.hpp"
#include "mdx/generated/Mdx/Module.hpp"

namespace mdx {

void install_core_module(MdxCoreServices& services) {
  mdx::generated::Mdx::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace mdx
