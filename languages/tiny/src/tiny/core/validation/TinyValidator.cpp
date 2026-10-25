#include "tiny/core/validation/TinyValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tiny/core/CoreServices.hpp"

namespace tiny {

namespace {
bool reserved(std::string_view name) {
  return name == "BEGIN" || name == "END" || name == "READ" || name == "WRITE";
}
}  // namespace

void TinyValidator::checkVarRef(const ast::VarRef& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (reserved(node.name))
    accept.error(node, "A reserved word cannot be used as a name.", "tiny.reserved-name")
        .property<&ast::VarRef::name>();
}

void TinyValidator::checkAssign(const ast::AssignStmt& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (reserved(node.target))
    accept.error(node, "A reserved word cannot be used as a name.", "tiny.reserved-name")
        .property<&ast::AssignStmt::target>();
}

void TinyValidator::checkRead(const ast::ReadStmt& node,
                              cursiv::ValidationAcceptor& accept) const {
  for (const auto name : node.names)
    if (reserved(name)) {
      accept.error(node, "A reserved word cannot be used as a name.", "tiny.reserved-name")
          .property<&ast::ReadStmt::names>();
      break;
    }
}

void register_validation_checks(TinyCoreServices& services) {
  services.validator = std::make_unique<const TinyValidator>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&TinyValidator::checkVarRef>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&TinyValidator::checkAssign>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&TinyValidator::checkRead>(validator)});
}

}  // namespace tiny
