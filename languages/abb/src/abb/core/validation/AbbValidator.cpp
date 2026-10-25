#include "abb/core/validation/AbbValidator.hpp"
#include "abb/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include <memory>
namespace abb {
namespace {
bool equalAscii(std::string_view a, std::string_view b) {
  if (a.size() != b.size())
    return false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    const char c = a[i] >= 'a' && a[i] <= 'z' ? a[i] - 'a' + 'A' : a[i];
    if (c != b[i])
      return false;
  }
  return true;
}
} // namespace
void AbbValidator::checkModule(const ast::Module &module,
                               cursiv::ValidationAcceptor &accept) const {
  if (module.parenthesized && module.options.empty())
    accept.error(module, "A module option list cannot be empty.",
                 "abb.module-options");
  for (auto option : module.options)
    if (!equalAscii(option, "SYSMODULE") && !equalAscii(option, "NOSTEPIN") &&
        !equalAscii(option, "VIEWONLY") && !equalAscii(option, "READONLY"))
      accept.error(module, "Unknown module option.", "abb.module-options")
          .property<&ast::Module::options>();
}
void AbbValidator::checkRoutine(const ast::Routine &procedure,
                                cursiv::ValidationAcceptor &accept) const {
  if (!procedure.parenthesized)
    accept.error(procedure, "A routine needs a parameter list in parentheses.",
                 "abb.procedure-parameters");
}
void AbbValidator::checkParameter(const ast::Parameter &parameter,
                                  cursiv::ValidationAcceptor &accept) const {
  if (parameter.type.empty())
    accept.error(parameter, "A parameter needs a data type.",
                 "abb.parameter-type");
}
void register_validation_checks(AbbCoreServices &services) {
  services.validator = std::make_unique<const AbbValidator>(services);
  using V = AbbValidator;
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkModule>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRoutine>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkParameter>(
           validator)});
}
} // namespace abb
