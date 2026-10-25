#include "suokif/core/validation/SuokifValidator.hpp"

#include <memory>
#include <string_view>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "suokif/core/CoreServices.hpp"

namespace suokif {
namespace {
std::string_view head(const ast::List& list) {
  if (list.items.empty()) return {};
  if (const auto* word = cursiv::ast_cast<ast::Word>(list.items[0])) return word->name;
  return {};
}
bool reserved(std::string_view name) {
  return name == "not" || name == "and" || name == "or" || name == "xor" ||
      name == "=>" || name == "<=>" || name == "forall" || name == "exists" || name == "equal" || name == "=";
}
bool sentence(const ast::Term* term) {
  if (cursiv::ast_cast<ast::Variable>(term)) return true;
  if (const auto* list = cursiv::ast_cast<ast::List>(term))
    return !list->items.empty() && !head(*list).ends_with("Fn");
  return false;
}
bool term(const ast::Term* value) {
  if (const auto* word = cursiv::ast_cast<ast::Word>(value)) return !reserved(word->name);
  const auto* list = cursiv::ast_cast<ast::List>(value);
  return !list || head(*list).ends_with("Fn");
}
bool variable_list(const ast::List& list) {
  const auto* parent = cursiv::ast_cast<ast::List>(list.parent);
  return parent && parent->items.size() > 1 && parent->items[1] == &list &&
      (head(*parent) == "forall" || head(*parent) == "exists");
}
}  // namespace

void SuokifValidator::checkDocument(const ast::Document& document,
                                    cursiv::ValidationAcceptor& accept) const {
  for (const auto* value : document.sentences) if (value && !sentence(value))
    accept.error(*value, "Expected a sentence.", "suokif.sentence");
}

void SuokifValidator::checkList(const ast::List& list,
                                cursiv::ValidationAcceptor& accept) const {
  const auto error = [&](std::string_view text) {
    accept.error(list, text, "suokif.arguments").property<&ast::List::items>();
  };
  if (variable_list(list)) {
    if (list.items.empty()) error("A quantifier requires at least one variable.");
    for (const auto* item : list.items) if (item && !cursiv::ast_cast<ast::Variable>(item))
      accept.error(*item, "Expected a quantified variable.", "suokif.variable");
    return;
  }
  if (list.items.empty()) { error("Expected a predicate or function name."); return; }
  const auto name = head(list);
  if (!cursiv::ast_cast<ast::Word>(list.items[0]) && !cursiv::ast_cast<ast::Variable>(list.items[0]))
    error("Expected a predicate, function name or variable.");
  const auto count = list.items.size() - 1;
  if (!count) error("Expected at least one argument.");
  if ((name == "not" && count != 1) ||
      ((name == "=>" || name == "<=>" || name == "equal" || name == "=" || name == "forall" || name == "exists") && count != 2) ||
      ((name == "and" || name == "or" || name == "xor") && count < 2))
    error("Unexpected number of arguments for this operator.");
  if (name == "=")
    accept.warning(list, "SUO-KIF spells equality as equal.", "suokif.equality");
  if (name == "forall" || name == "exists") {
    if (count && !cursiv::ast_cast<ast::List>(list.items[1])) error("Expected a list of quantified variables.");
    if (count > 1 && !sentence(list.items[2])) error("Expected a sentence after the variable list.");
  } else if (name == "not" || name == "and" || name == "or" || name == "xor" || name == "=>" || name == "<=>") {
    for (std::size_t i = 1; i < list.items.size(); ++i) if (!sentence(list.items[i]))
      accept.error(*list.items[i], "Expected a sentence.", "suokif.sentence");
  } else if (name == "equal" || name == "=") {
    for (std::size_t i = 1; i < list.items.size(); ++i) if (!term(list.items[i]))
      accept.error(*list.items[i], "Expected a term.", "suokif.term");
  } else {
    for (std::size_t i = 1; i < list.items.size(); ++i)
      if (const auto* word = cursiv::ast_cast<ast::Word>(list.items[i]); word && reserved(word->name))
        accept.error(*word, "A logical operator cannot be used as a term.", "suokif.term").property<&ast::Word::name>();
  }
}

void register_validation_checks(SuokifCoreServices& services) {
  services.validator = std::make_unique<const SuokifValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&SuokifValidator::checkDocument>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&SuokifValidator::checkList>(*services.validator)});
}
}  // namespace suokif
