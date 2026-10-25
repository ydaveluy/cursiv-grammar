#include "racketisl/core/validation/RacketIslValidator.hpp"

#include <memory>
#include <string>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "racketisl/core/CoreServices.hpp"

namespace racketisl {
namespace {
void expression(const ast::Datum *datum, cursiv::ValidationAcceptor &accept) {
  if (!datum)
    return;
  if (cursiv::ast_cast<ast::Prefix>(datum))
    return;
  const auto *list = cursiv::ast_cast<ast::List>(datum);
  if (!list || list->opener.size() != 1)
    return;
  std::vector<const ast::Datum *> items;
  for (const auto *item : list->items)
    if (!cursiv::ast_cast<ast::Discard>(item) &&
        !cursiv::ast_cast<ast::Directive>(item))
      items.push_back(item);
  if (items.empty())
    return;
  const auto *head = cursiv::ast_cast<ast::Atom>(items[0]);
  if (head && (head->text == "quote" || head->text == "quasiquote" ||
               head->text == "syntax" || head->text == "quasisyntax"))
    return;
  if (head && (head->text == "and" || head->text == "or") && items.size() < 3)
    accept.error(*list,
                 std::string(head->text) + " expects at least two operands.",
                 "racketisl.boolean.arity");
  for (const auto *item : items)
    expression(item, accept);
  expression(list->tail, accept);
}
} // namespace
void RacketIslValidator::checkList(const ast::List &list,
                                   cursiv::ValidationAcceptor &accept) const {
  if (list.opener.empty() || list.closer.empty())
    return;
  const char opening = list.opener.back();
  const char expected = opening == '(' ? ')' : opening == '[' ? ']' : '}';
  if (list.closer.front() != expected)
    accept
        .error(list,
               std::string("Expected '") + expected + "' to close this list.",
               "racketisl.delimiter")
        .property<&ast::List::closer>();
}
void RacketIslValidator::checkProgram(
    const ast::Program &program, cursiv::ValidationAcceptor &accept) const {
  for (const auto *form : program.forms)
    expression(form, accept);
}
void register_validation_checks(RacketIslCoreServices &services) {
  using V = RacketIslValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkList>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProgram>(
           validator)});
}
} // namespace racketisl
