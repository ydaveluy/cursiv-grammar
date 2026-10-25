// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "xquery/core/CoreServices.hpp"
#include "xquery/core/validation/XQueryValidator.hpp"
#include "xquery/generated/XQuery/Module.hpp"

namespace xquery {

void install_core_module(XQueryCoreServices& services) {
  xquery::generated::XQuery::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace xquery
