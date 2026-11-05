#include "ebnf/validation/EbnfValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "ebnf/CoreServices.hpp"

namespace ebnf {

void register_validation_checks(EbnfCoreServices& services) {
  services.validator = std::make_unique<const EbnfValidator>(services);
}

}  // namespace ebnf
