#include "datalog/core/validation/DatalogValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "datalog/core/CoreServices.hpp"

namespace datalog {

void DatalogValidator::checkClauseHead(
    const ast::Clause& clause, cursiv::ValidationAcceptor& accept) const {
  if (cursiv::ast_cast<ast::ExternalLiteral>(clause.head))
    accept
        .error(clause, "An external binding cannot be a clause head.",
               "datalog.head")
        .property<&ast::Clause::head>();
}

void register_validation_checks(DatalogCoreServices& services) {
  using V = DatalogValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkClauseHead>(
          *services.validator)});
}

}  // namespace datalog
