#include "bnf/core/validation/BnfValidator.hpp"

#include <memory>

#include "bnf/core/CoreServices.hpp"

namespace bnf {

void register_validation_checks(BnfCoreServices& services) {
  services.validator = std::make_unique<const BnfValidator>(services);
}

}  // namespace bnf
