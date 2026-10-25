// What this language installs over the generated module: what its author
// writes. Everything the grammar derives is in the generated one.

#include <memory>

#include "xsmp/core/CoreServices.hpp"
#include "xsmp/core/XsmpScopeComputation.hpp"
#include "xsmp/generated/Xsmp/Module.hpp"

namespace xsmp {

void install_core_module(XsmpCoreServices& services) {
  xsmp::generated::Xsmp::install_generated_module(services);
  services.scopeComputation = std::make_unique<XsmpScopeComputation>(services);
}

}  // namespace xsmp
