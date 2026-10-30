#include "clu/core/validation/CluValidator.hpp"

#include <format>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "clu/core/CoreServices.hpp"

namespace clu {
namespace {
bool same_id(std::string_view left, std::string_view right) {
  if (left.size() != right.size()) return false;
  const auto fold = [](char c) { return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c; };
  for (std::size_t i = 0; i < left.size(); ++i)
    if (fold(left[i]) != fold(right[i])) return false;
  return true;
}
}  // namespace

void CluValidator::checkRoutineEndName(
    const ast::Routine& routine, cursiv::ValidationAcceptor& accept) const {
  if (!routine.endName.empty() && !same_id(routine.endName, routine.name))
    accept
        .error(routine,
               std::format("End name '{}' does not match '{}'.",
                           routine.endName, routine.name))
        .property<&ast::Routine::endName>();
}

void CluValidator::checkClusterEndName(
    const ast::Cluster& cluster, cursiv::ValidationAcceptor& accept) const {
  if (!cluster.endName.empty() && !same_id(cluster.endName, cluster.name))
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
