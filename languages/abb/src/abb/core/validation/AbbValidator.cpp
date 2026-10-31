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
  if (!cursiv::ast_cast<ast::Trap>(&procedure) && !procedure.parenthesized)
    accept.error(procedure, "A routine needs a parameter list in parentheses.",
                 "abb.procedure-parameters");
}
void AbbValidator::checkParameter(const ast::Parameter &parameter,
                                  cursiv::ValidationAcceptor &accept) const {
  if (parameter.type.empty())
    accept.error(parameter, "A parameter needs a data type.",
                 "abb.parameter-type");
}
void AbbValidator::checkControl(const ast::Statement &statement,
                                cursiv::ValidationAcceptor &accept) const {
  const ast::Routine* routine = nullptr;
  bool inLoop = false, inError = false;
  for (const auto* parent = statement.parent; parent; parent = parent->parent) {
    inLoop |= cursiv::ast_cast<ast::For>(parent) != nullptr ||
              cursiv::ast_cast<ast::While>(parent) != nullptr;
    inError |= cursiv::ast_cast<ast::ErrorHandler>(parent) != nullptr;
    if ((routine = cursiv::ast_cast<ast::Routine>(parent))) break;
  }
  if ((cursiv::ast_cast<ast::Break>(&statement) ||
       cursiv::ast_cast<ast::Continue>(&statement)) && !inLoop)
    accept.error(statement, "BREAK and CONTINUE require an enclosing loop.",
                 "abb.loop-control");
  if ((cursiv::ast_cast<ast::Retry>(&statement) ||
       cursiv::ast_cast<ast::TryNext>(&statement)) && !inError)
    accept.error(statement, "RETRY and TRYNEXT require an error handler.",
                 "abb.error-control");
  if (const auto* raised = cursiv::ast_cast<ast::Raise>(&statement))
    if (inError ? raised->error != nullptr : raised->error == nullptr)
      accept.error(statement,
                   "An error handler re-raises without a number; elsewhere RAISE needs one.",
                   "abb.raise-context");
  if (const auto* returned = cursiv::ast_cast<ast::Return>(&statement))
    if (routine && ((cursiv::ast_cast<ast::Function>(routine) != nullptr) !=
                        (returned->value != nullptr)))
      accept.error(statement,
                   "Only a function returns a value, and its RETURN needs one.",
                   "abb.return-value");
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
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkControl>(
           validator)});
}
} // namespace abb
