#include "quakemap/core/validation/QuakemapValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "quakemap/core/CoreServices.hpp"

namespace quakemap {

void QuakemapValidator::checkPlaneCoordCount(
    const ast::BrushLine& line, cursiv::ValidationAcceptor& accept) const {
  if (line.coords.size() != 3)
    accept.warning(line, "A brush plane is defined by exactly 3 points.");
}

void QuakemapValidator::checkCoordComponents(
    const ast::Coord& coord, cursiv::ValidationAcceptor& accept) const {
  if (coord.values.size() != 3)
    accept.warning(coord, "A point has exactly 3 components (x y z).");
}

void register_validation_checks(QuakemapCoreServices& services) {
  using V = QuakemapValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkPlaneCoordCount>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkCoordComponents>(validator)});
}

}  // namespace quakemap
