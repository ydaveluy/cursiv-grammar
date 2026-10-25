#include "sapf/core/validation/SapfValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sapf/core/CoreServices.hpp"

namespace sapf {

void SapfValidator::checkUniqueParams(const ast::FunctionLiteral& function,
                                      cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> seen;
  for (const std::string_view param : function.params)
    if (!seen.insert(param).second)
      accept.error(function, "Duplicate parameter name.");
}

void register_validation_checks(SapfCoreServices& services) {
  using V = SapfValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueParams>(
          validator)});
}

}  // namespace sapf
