#include "mumps/core/validation/MumpsValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "mumps/core/CoreServices.hpp"

namespace mumps {

void MumpsValidator::checkOtherCommand(
    const ast::OtherCommand& node, cursiv::ValidationAcceptor& accept) const {
  std::string name(node.keyword);
  for (auto& c : name)
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  if ((name == "CLOSE" || name == "C" || name == "GOTO" || name == "G" ||
       name == "JOB" || name == "J" || name == "OPEN" || name == "O" ||
       name == "USE" || name == "U") &&
      node.argumentText.empty())
    accept.error(node, "This command requires an argument.");
}

void register_validation_checks(MumpsCoreServices& services) {
  using V = MumpsValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkOtherCommand>(
          *services.validator)});
}

}  // namespace mumps
