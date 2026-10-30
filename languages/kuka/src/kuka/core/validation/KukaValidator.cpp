#include "kuka/core/validation/KukaValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "kuka/core/CoreServices.hpp"

namespace kuka {

void KukaValidator::checkUniqueParams(const ast::Routine& routine,
                                      cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string, std::vector<const ast::Param*>> names;
  for (const ast::Param* const param : routine.params)
    if (param != nullptr && param->name != nullptr) {
      std::string folded(param->name->id);
      for (auto& c : folded) if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
      names[folded].push_back(param);
    }

  for (const auto& [name, params] : names) {
    if (params.size() <= 1) continue;
    const std::string message = "Duplicate parameter name: " + std::string(name);
    for (const ast::Param* const param : params)
      accept.error(*param->name, message).property<&ast::VarName::id>();
  }
}

void register_validation_checks(KukaCoreServices& services) {
  using V = KukaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkUniqueParams>(validator)});
}

}  // namespace kuka
