#include "json/core/validation/JsonValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "json/core/CoreServices.hpp"

namespace json {

void JsonValidator::checkUniqueMemberNames(
    const ast::Object &object, cursiv::ValidationAcceptor &accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Member *>> names;
  for (const ast::Member *const member : object.members)
    if (member != nullptr)
      names[member->name].push_back(member);

  for (const auto &[name, members] : names) {
    if (members.size() <= 1)
      continue;
    const std::string message = "Duplicate object key: " + std::string(name);
    for (const ast::Member *const member : members)
      accept.warning(*member, message).property<&ast::Member::name>();
  }
}

void register_validation_checks(JsonCoreServices &services) {
  using V = JsonValidator;
  services.validator = std::make_unique<const V>(services);
  const V &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueMemberNames>(validator)});
}

} // namespace json
