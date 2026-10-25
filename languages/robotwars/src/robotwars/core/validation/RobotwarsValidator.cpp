#include "robotwars/core/validation/RobotwarsValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "robotwars/core/CoreServices.hpp"

namespace robotwars {

// GOSUB/GOTO resolve a label by name against every one declared in the
// program: two labels sharing a name make that resolution ambiguous.
void RobotwarsValidator::checkUniqueLabels(
    const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Label*>> names;
  for (const ast::Item* const item : program.items) {
    if (item->type != &cursiv::ast_type_tag<ast::Label>) continue;
    const auto* const label = static_cast<const ast::Label*>(item);
    names[label->name].push_back(label);
  }

  for (const auto& [name, labels] : names) {
    if (labels.size() <= 1) continue;
    const std::string message = "Duplicate label: " + std::string(name);
    for (const ast::Label* const label : labels)
      accept.error(*label, message).property<&ast::Label::name>();
  }
}

void register_validation_checks(RobotwarsCoreServices& services) {
  using V = RobotwarsValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueLabels>(
          validator)});
}

}  // namespace robotwars
