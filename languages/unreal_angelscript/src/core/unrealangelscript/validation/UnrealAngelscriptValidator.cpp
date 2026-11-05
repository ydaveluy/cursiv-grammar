#include "unrealangelscript/validation/UnrealAngelscriptValidator.hpp"

#include <memory>

#include "cursiv/validation/ValidationRegistry.hpp"
#include "unrealangelscript/CoreServices.hpp"

namespace unrealangelscript {

void register_validation_checks(UnrealAngelscriptCoreServices& services) {
  using V = UnrealAngelscriptValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace unrealangelscript
