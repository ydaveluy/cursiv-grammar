#include "erlang/validation/ErlangValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "erlang/CoreServices.hpp"

namespace erlang {

// Nothing to register: this port has no check beyond what the grammar
// already enforces.
void register_validation_checks(ErlangCoreServices& services) {
  services.validator = std::make_unique<const ErlangValidator>(services);
}

}  // namespace erlang
