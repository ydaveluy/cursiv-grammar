#include "csharp/core/validation/CSharpValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "csharp/core/CoreServices.hpp"

namespace csharp {

// No checks yet: the registry may register nothing.
void register_validation_checks(CSharpCoreServices& services) {
  services.validator = std::make_unique<const CSharpValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace csharp
