#include "maml/core/validation/MamlValidator.hpp"

#include <memory>
#include <unordered_set>
#include "cursiv/core/validation/ValidationRegistry.hpp"

#include "maml/core/CoreServices.hpp"

namespace maml {

void MamlValidator::checkObject(const ast::Object& object,
                                cursiv::ValidationAcceptor& accept) const {
  std::unordered_set<std::string_view> keys;
  for (const auto* member : object.members)
    if (!keys.insert(member->key).second)
      accept.error(*member, "An object cannot contain duplicate decoded keys.", "maml.duplicate-key")
          .property<&ast::Member::key>();
}

void register_validation_checks(MamlCoreServices& services) {
  services.validator = std::make_unique<const MamlValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&MamlValidator::checkObject>(*services.validator)});
}

}  // namespace maml
