// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "csharp/core/CoreServices.hpp"
#include "csharp/core/validation/CSharpValidator.hpp"
#include "csharp/generated/CSharp/Module.hpp"

namespace csharp {

void install_core_module(CSharpCoreServices& services) {
  csharp::generated::CSharp::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace csharp
