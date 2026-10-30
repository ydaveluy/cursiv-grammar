#include "molecule/core/validation/MoleculeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "molecule/core/CoreServices.hpp"

namespace molecule {

void MoleculeValidator::checkStructure(
    const ast::Structure& structure, cursiv::ValidationAcceptor& accept) const {
  if (structure.count && *structure.count == 0)
    accept.error(structure, "An atom or group count must be positive.",
                 "molecule.count")
        .property<&ast::Structure::count>();
}

void MoleculeValidator::checkPart(
    const ast::Part& part, cursiv::ValidationAcceptor& accept) const {
  if (part.multiplier && *part.multiplier == 0)
    accept.error(part, "A formula coefficient must be positive.",
                 "molecule.coefficient")
        .property<&ast::Part::multiplier>();
}

void register_validation_checks(MoleculeCoreServices& services) {
  services.validator = std::make_unique<const MoleculeValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &MoleculeValidator::checkStructure>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &MoleculeValidator::checkPart>(validator)});
}

}  // namespace molecule
