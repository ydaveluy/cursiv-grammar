#include "stacktrace/core/validation/StacktraceValidator.hpp"

#include <memory>

#include "stacktrace/core/CoreServices.hpp"

namespace stacktrace {

void register_validation_checks(StacktraceCoreServices& services) {
  services.validator = std::make_unique<const StacktraceValidator>(services);
}

}  // namespace stacktrace
