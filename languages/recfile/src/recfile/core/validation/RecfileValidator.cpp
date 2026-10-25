#include "recfile/core/validation/RecfileValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "recfile/core/CoreServices.hpp"
#include "recfile/core/Value.hpp"

namespace recfile {

void RecfileValidator::checkField(const ast::Field &field,
                                  cursiv::ValidationAcceptor &accept) const {
  if (!field_name(field.key))
    accept
        .error(field,
               "A field name starts with a letter or '%' and continues with "
               "letters, digits or underscores.",
               "recfile.name")
        .property<&ast::Field::key>();
  if (!field.indentation.empty())
    if (const auto *record = cursiv::ast_cast<ast::Record>(field.parent);
        record && !record->lines.empty() && record->lines[0] != &field)
      accept
          .error(
              field,
              "Only the first field of a record may have leading indentation.",
              "recfile.indentation")
          .property<&ast::Field::indentation>();
  if (!field.terminated && !field.parts.empty()) {
    const auto &last = *field.parts.back();
    if (last.text.ends_with('\\') ||
        (last.join == ast::Join::newline && last.text.empty()))
      accept.error(field, "The field ends in an incomplete continuation.",
                   "recfile.continuation");
    if (field.parts.size() == 1 && last.text.empty())
      accept.warning(
          field,
          "An empty final field needs a newline or a padding space after ':'.",
          "recfile.empty-eof");
  }
}

void RecfileValidator::checkComment(const ast::Comment &comment,
                                    cursiv::ValidationAcceptor &accept) const {
  if (!comment.indentation.empty() &&
      cursiv::ast_cast<ast::Record>(comment.parent))
    accept
        .error(comment,
               "A comment inside a record must start in the first column.",
               "recfile.indentation")
        .property<&ast::Comment::indentation>();
}

void RecfileValidator::checkRawLine(const ast::RawLine &line,
                                    cursiv::ValidationAcceptor &accept) const {
  accept.error(line, "Expected a field name followed by ':', or a comment.",
               "recfile.line");
}

void register_validation_checks(RecfileCoreServices &services) {
  services.validator = std::make_unique<const RecfileValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &RecfileValidator::checkField>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &RecfileValidator::checkComment>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &RecfileValidator::checkRawLine>(*services.validator)});
}

} // namespace recfile
