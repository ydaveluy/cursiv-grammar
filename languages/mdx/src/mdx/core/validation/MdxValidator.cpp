#include "mdx/core/validation/MdxValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "mdx/core/CoreServices.hpp"

namespace mdx {

// Two axes named the same (`... ON COLUMNS, ... ON COLUMNS`) is legal by
// this grammar but never a valid query: the axis position is what a
// provider actually keys on.
void MdxValidator::checkDuplicateAxisNames(
    const ast::SelectStatement& select, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::AxisSpec*>> byName;
  for (const ast::AxisSpec* const axis : select.axes)
    if (axis != nullptr) byName[axis->axisName].push_back(axis);

  for (const auto& [name, axes] : byName) {
    if (axes.size() <= 1) continue;
    const std::string message = "Duplicate axis: " + std::string(name);
    for (const ast::AxisSpec* const axis : axes)
      accept.error(*axis, message).property<&ast::AxisSpec::axisName>();
  }
}

void register_validation_checks(MdxCoreServices& services) {
  using V = MdxValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkDuplicateAxisNames>(validator)});
}

}  // namespace mdx
