#include "molecule/core/validation/MoleculeValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "molecule/core/CoreServices.hpp"

namespace molecule {

void register_validation_checks(MoleculeCoreServices& services) {
  services.validator = std::make_unique<const MoleculeValidator>(services);
  services.validationRegistry->registerChecks({});
}

}  // namespace molecule
