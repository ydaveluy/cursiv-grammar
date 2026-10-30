#include "elixir/core/validation/ElixirValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "elixir/core/CoreServices.hpp"

namespace elixir {

namespace {
template<class Node>
void checkHeredocHeader(const Node& node, cursiv::ValidationAcceptor& accept) {
  const auto text = node.texts.empty() ? std::string_view{} : node.texts[0];
  const auto first = text.find_first_not_of(" \t");
  if (first == std::string_view::npos ||
      (text[first] != '\n' && text.substr(first, 2) != "\r\n"))
    accept.error(node, "A heredoc opener requires whitespace followed by a newline.",
                 "elixir.heredoc-header");
}
template<class Node, class Positional, class Keywords>
void checkOrder(const Node& node, const Positional& items, const Keywords& keys,
                cursiv::ValidationAcceptor& accept) {
  if (items.empty() || keys.empty()) return;
  const auto first = accept.spanOf(*keys[0]).begin;
  for (auto item : items)
    if (item && accept.spanOf(*item).begin > first) {
      accept.error(node, "Keyword options must come after positional values", "elixir.keyword-order");
      return;
    }
}
}
void ElixirValidator::checkMultiString(const ast::MultiStringLit& node, cursiv::ValidationAcceptor& accept) const {
  checkHeredocHeader(node, accept);
}
void ElixirValidator::checkMultiCharlist(const ast::MultiCharlistLit& node, cursiv::ValidationAcceptor& accept) const {
  checkHeredocHeader(node, accept);
}
void ElixirValidator::checkSigil(const ast::SigilLit& node, cursiv::ValidationAcceptor& accept) const {
  if (node.delimiter.size() == 3) checkHeredocHeader(node, accept);
}
void ElixirValidator::checkRange(const ast::BinaryExpr& node, cursiv::ValidationAcceptor& accept) const {
  if (node.op != ast::Operator::range_step || !node.left) return;
  const auto* range = cursiv::ast_cast<ast::BinaryExpr>(node.left);
  if (!range || range->op != ast::Operator::range)
    accept.error(node, "A range step must follow a range, for example 1..9//2", "elixir.range-step")
        .property<&ast::BinaryExpr::left>();
}
void ElixirValidator::checkCall(const ast::CallExpr& node, cursiv::ValidationAcceptor& accept) const {
  checkOrder(node, node.args, node.options, accept);
}
void ElixirValidator::checkList(const ast::ListExpr& node, cursiv::ValidationAcceptor& accept) const {
  checkOrder(node, node.items, node.keywords, accept);
}
void ElixirValidator::checkMap(const ast::MapExpr& node, cursiv::ValidationAcceptor& accept) const {
  checkOrder(node, node.entries, node.keywords, accept);
}
void register_validation_checks(ElixirCoreServices& services) {
  services.validator = std::make_unique<const ElixirValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkMultiString>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkMultiCharlist>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkSigil>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkRange>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkCall>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkList>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&ElixirValidator::checkMap>(*services.validator)});
}

}  // namespace elixir
