#include "newick/core/validation/NewickValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "newick/core/CoreServices.hpp"
#include "newick/core/LengthValue.hpp"
namespace newick {
void NewickValidator::checkClade(const ast::Clade& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (n.length && !length_value(*n.length))
    a.error(n, "a branch length must be a number")
        .property<&ast::Clade::length>();
}
void register_validation_checks(NewickCoreServices& services) {
  services.validator = std::make_unique<const NewickValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &NewickValidator::checkClade>(*services.validator)});
}
}  // namespace newick
