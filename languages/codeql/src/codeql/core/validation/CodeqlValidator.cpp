#include "codeql/core/validation/CodeqlValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "codeql/core/CoreServices.hpp"

namespace codeql {

void register_validation_checks(CodeqlCoreServices& services) {
  using V = CodeqlValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace codeql
