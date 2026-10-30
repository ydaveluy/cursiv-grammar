#include "robotwars/core/validation/RobotwarsValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "robotwars/core/CoreServices.hpp"

namespace robotwars {

void RobotwarsValidator::checkNumber(
    const ast::NumberLit& number, cursiv::ValidationAcceptor& accept) const {
  if (number.value > 1024)
    accept.error(number, "RobotWar literals must be between -1024 and +1024.",
                 "robotwars.number-range").property<&ast::NumberLit::value>();
}

void RobotwarsValidator::checkLabel(
    const ast::Label& label, cursiv::ValidationAcceptor& accept) const {
  if (label.name.size() >= 32)
    accept.error(label, "RobotWar labels must have fewer than 32 characters.",
                 "robotwars.label-length").property<&ast::Label::name>();
  std::string name(label.name);
  for (char& c : name) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  for (const std::string_view reserved : {"TO", "IF", "GOTO", "GOSUB", "ENDSUB", "AIM",
      "SHOT", "RADAR", "DAMAGE", "SPEEDX", "SPEEDY", "RANDOM", "INDEX", "DATA"}) {
    if (name == reserved) {
      accept.error(label, "RobotWar labels cannot be register or command names.",
                   "robotwars.label-reserved").property<&ast::Label::name>();
      break;
    }
  }
}

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
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLabel>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueLabels>(
          validator)});
}

}  // namespace robotwars
