#include "turing/core/validation/TuringValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "turing/core/CoreServices.hpp"

namespace turing {

void TuringValidator::checkSubprogramEndNameMatches(
    const ast::SubprogramDecl& subprogram,
    cursiv::ValidationAcceptor& accept) const {
  if (!subprogram.endName.empty() && subprogram.endName != subprogram.name)
    accept
        .error(subprogram, "'end " + std::string(subprogram.endName) +
                                "' does not match '" +
                                std::string(subprogram.name) + "'")
        .property<&ast::SubprogramDecl::endName>();
}

void register_validation_checks(TuringCoreServices& services) {
  using V = TuringValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSubprogramEndNameMatches>(validator)});
}

}  // namespace turing
