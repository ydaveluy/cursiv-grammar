#include "eiffel/core/validation/EiffelValidator.hpp"

#include <cctype>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "eiffel/core/CoreServices.hpp"

namespace eiffel {

void EiffelValidator::checkClassNameIsUppercase(
    const ast::ClassDecl& cls, cursiv::ValidationAcceptor& accept) const {
  for (const char c : cls.name)
    if (std::isalpha(static_cast<unsigned char>(c)) && std::islower(static_cast<unsigned char>(c))) {
      accept.warning(cls, "Class names are conventionally uppercase.")
          .property<&ast::ClassDecl::name>();
      return;
    }
}

void register_validation_checks(EiffelCoreServices& services) {
  using V = EiffelValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkClassNameIsUppercase>(validator)});
}

}  // namespace eiffel
