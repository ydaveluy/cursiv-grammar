#include "csv/core/validation/CsvValidator.hpp"

#include <memory>

#include "csv/core/CoreServices.hpp"
#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace csv {

void CsvValidator::checkRowFieldCount(
    const ast::Row& row, cursiv::ValidationAcceptor& accept) const {
  if (row.parent == nullptr ||
      row.parent->type != &cursiv::ast_type_tag<ast::Doc>)
    return;
  const auto& doc = *static_cast<const ast::Doc*>(row.parent);
  if (doc.rows.empty() || &row == doc.rows[0]) return;
  if (row.fields.size() != doc.rows[0]->fields.size())
    accept.warning(row, "Row has a different number of fields than the "
                        "first row.");
}

void register_validation_checks(CsvCoreServices& services) {
  using V = CsvValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRowFieldCount>(validator)});
}

}  // namespace csv
