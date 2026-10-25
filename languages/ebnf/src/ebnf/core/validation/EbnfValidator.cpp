#include "ebnf/core/validation/EbnfValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ebnf/core/CoreServices.hpp"

namespace ebnf {

void register_validation_checks(EbnfCoreServices& services) {
  services.validator = std::make_unique<const EbnfValidator>(services);
}

}  // namespace ebnf
