#include "ruleworks/core/validation/RuleworksValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "ruleworks/core/CoreServices.hpp"

namespace ruleworks {

void RuleworksValidator::checkUniqueAttributes(
    const ast::ObjectClassDecl& objectClass,
    cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::AttributeDecl*>>
      byName;
  for (const ast::AttributeDecl* const attribute : objectClass.attributes)
    if (attribute != nullptr) byName[attribute->name].push_back(attribute);

  for (const auto& [name, attributes] : byName) {
    if (attributes.size() <= 1) continue;
    const std::string message =
        "Duplicate attribute: ^" + std::string(name);
    for (const ast::AttributeDecl* const attribute : attributes)
      accept.error(*attribute, message).property<&ast::AttributeDecl::name>();
  }
}

void RuleworksValidator::checkBuiltinArity(
    const ast::Call& node, cursiv::ValidationAcceptor& accept) const {
  std::string name(node.name);
  for (auto& c : name)
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  const auto n = node.args.size();
  bool invalid = false;
  if (name == "CRLF" || name == "GENINT")
    invalid = n != 0;
  else if (name == "ACCEPT-ATOM" || name == "GENATOM")
    invalid = n > 1;
  else if (name == "ACCEPTLINE-COMPOUND")
    invalid = n > 2;
  else if (name == "FLOAT" || name == "INTEGER" || name == "IS-OPEN" ||
           name == "LENGTH" || name == "RJUST" || name == "SYMBOL" ||
           name == "SYMBOL-LENGTH" || name == "TABTO")
    invalid = n != 1;
  if (invalid)
    accept.error(node,
                 "Incorrect number of arguments for builtin " + name + ".");
}

void register_validation_checks(RuleworksCoreServices& services) {
  using V = RuleworksValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &RuleworksValidator::checkBuiltinArity>(*services.validator)});
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueAttributes>(validator)});
}

}  // namespace ruleworks
