#include "clu/core/validation/CluValidator.hpp"

#include <format>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "clu/core/CoreServices.hpp"

namespace clu {

void CluValidator::checkRoutineEndName(
    const ast::Routine& routine, cursiv::ValidationAcceptor& accept) const {
  if (!routine.endName.empty() && routine.endName != routine.name)
    accept
        .error(routine,
               std::format("End name '{}' does not match '{}'.",
                           routine.endName, routine.name))
        .property<&ast::Routine::endName>();
}

void CluValidator::checkClusterEndName(
    const ast::Cluster& cluster, cursiv::ValidationAcceptor& accept) const {
  if (!cluster.endName.empty() && cluster.endName != cluster.name)
    accept
        .error(cluster,
               std::format("End name '{}' does not match '{}'.",
                           cluster.endName, cluster.name))
        .property<&ast::Cluster::endName>();
}

void register_validation_checks(CluCoreServices& services) {
  using V = CluValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkRoutineEndName>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkClusterEndName>(validator)});
}

}  // namespace clu
