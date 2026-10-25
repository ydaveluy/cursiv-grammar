#include "qif/core/validation/QifValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "qif/core/CoreServices.hpp"

namespace qif {

void QifValidator::checkRecords(const ast::Document &document,
                                cursiv::ValidationAcceptor &accept) const {
  std::string_view header;
  for (const auto *entry : document.entries) {
    if (const auto *next = cursiv::ast_cast<ast::Header>(entry)) {
      header = next->text;
      if (header.empty())
        accept.error(*next, "A QIF header needs a name.", "qif.header");
      continue;
    }
    const auto *record = cursiv::ast_cast<ast::Record>(entry);
    if (!record)
      continue;
    if (header.empty())
      accept.error(*record, "A QIF record needs a section header.",
                   "qif.header");
    if (!record->terminated)
      accept.error(*record, "A QIF record must end with ^.", "qif.record-end");
    const bool transaction = header == "Type:Bank" || header == "Type:Cash" ||
                             header == "Type:Ccard" || header == "Type:Oth A" ||
                             header == "Type:Oth L" ||
                             header == "Type:Invoice" ||
                             header == "Type:Memorized";
    bool split = false;
    for (const auto *field : record->fields) {
      if (field->code == "S")
        split = true;
      if (transaction && !split && (field->code == "E" || field->code == "$"))
        accept.error(*field, "A split field needs a preceding S field.",
                     "qif.split");
    }
  }
}

void register_validation_checks(QifCoreServices &services) {
  services.validator = std::make_unique<const QifValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &QifValidator::checkRecords>(*services.validator)});
}

} // namespace qif
