// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "stacktrace/core/CoreServices.hpp"
#include "stacktrace/core/validation/StacktraceValidator.hpp"
#include "stacktrace/generated/Stacktrace/Module.hpp"

namespace stacktrace {

void install_core_module(StacktraceCoreServices& services) {
  stacktrace::generated::Stacktrace::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace stacktrace
