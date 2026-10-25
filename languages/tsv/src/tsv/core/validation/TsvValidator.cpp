#include "tsv/core/validation/TsvValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "tsv/core/CoreServices.hpp"

namespace tsv {

void TsvValidator::checkRowFieldCount(
    const ast::Document& doc, cursiv::ValidationAcceptor& accept) const {
  if (doc.header == nullptr) return;
  const std::size_t want = doc.header->fields.size();
  for (const ast::Row* const row : doc.rows) {
    if (row == nullptr || row->fields.size() == want) continue;
    accept.warning(*row, "Row has " + std::to_string(row->fields.size()) +
                             " field(s), header declares " +
                             std::to_string(want) + ".");
  }
}

void register_validation_checks(TsvCoreServices& services) {
  using V = TsvValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRowFieldCount>(validator)});
}

}  // namespace tsv
