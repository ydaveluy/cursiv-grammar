#include "bibtex/core/validation/BibtexValidator.hpp"
#include <algorithm>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "bibtex/core/CoreServices.hpp"

namespace bibtex {
namespace {
template <auto Close, class Node>
void delimiter(const Node& node, cursiv::ValidationAcceptor& accept) {
  if ((node.open == "{" && node.close != "}") ||
      (node.open == "(" && node.close != ")"))
    accept.error(node, "The closing delimiter does not match the opener.")
        .template property<Close>().code("bibtex.delimiter");
}
}
void BibtexValidator::checkUniqueCitekeys(
    const ast::Document& doc, cursiv::ValidationAcceptor& accept) const {
  std::unordered_map<std::string_view, std::vector<const ast::Entry*>> keys;
  for (const auto* declaration : doc.declarations)
    if (const auto* entry = cursiv::ast_cast<ast::Entry>(declaration);
        entry && !entry->citekey.empty())
      keys[entry->citekey].push_back(entry);
  for (const auto& [key, entries] : keys) {
    if (entries.size() <= 1) continue;
    for (const auto* entry : entries)
      accept.error(*entry, "Duplicate citation key: " + std::string(key))
          .property<&ast::Entry::citekey>().code("bibtex.duplicate-key");
  }
}
void BibtexValidator::checkEntry(
    const ast::Entry& node, cursiv::ValidationAcceptor& accept) const {
  delimiter<&ast::Entry::close>(node, accept);
  if (std::all_of(node.type.begin(), node.type.end(),
                  [](char c) { return c >= '0' && c <= '9'; }))
    accept.error(node, "An entry type must be a name.")
        .property<&ast::Entry::type>().code("bibtex.entry-type");
  if (node.citekey.empty())
    accept.error(node, "A citation key is required.").code("bibtex.key");
  else if (!node.hasKeySeparator)
    accept.error(node, "The citation key needs a following comma.")
        .property<&ast::Entry::citekey>().code("bibtex.separator");
}
void BibtexValidator::checkString(
    const ast::StringDef& node, cursiv::ValidationAcceptor& accept) const {
  delimiter<&ast::StringDef::close>(node, accept);
}
void BibtexValidator::checkPreamble(
    const ast::Preamble& node, cursiv::ValidationAcceptor& accept) const {
  delimiter<&ast::Preamble::close>(node, accept);
}
void BibtexValidator::checkComment(
    const ast::Comment& node, cursiv::ValidationAcceptor& accept) const {
  if (node.raw.size() < 2) return;
  int depth = 0;
  for (char c : node.raw.substr(1, node.raw.size() - 2)) {
    if (c == '{') ++depth;
    if (c == '}') --depth;
    if (depth < 0) break;
  }
  if (depth != 0)
    accept.error(node, "The comment contains unbalanced braces.")
        .property<&ast::Comment::raw>().code("bibtex.comment-braces");
}
void BibtexValidator::checkField(
    const ast::Field& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.key.empty() && node.key.front() >= '0' && node.key.front() <= '9')
    accept.error(node, "A field or macro name cannot start with a digit.")
        .property<&ast::Field::key>().code("bibtex.field-name");
}
void BibtexValidator::checkValue(
    const ast::Value& node, cursiv::ValidationAcceptor& accept) const {
  if (node.parts.empty())
    accept.error(node, "A value is required.").code("bibtex.value");
}
void register_validation_checks(BibtexCoreServices& services) {
  using V = BibtexValidator;
  services.validator = std::make_unique<const V>(services);
  const V& v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkUniqueCitekeys>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkEntry>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPreamble>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkComment>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkField>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkValue>(v)});
}
}  // namespace bibtex
