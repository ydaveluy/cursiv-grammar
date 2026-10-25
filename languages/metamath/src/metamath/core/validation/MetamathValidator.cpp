#include "metamath/core/validation/MetamathValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "metamath/core/CoreServices.hpp"

namespace metamath {
namespace {
bool validLabel(std::string_view label) {
  if (label.empty())
    return false;
  for (char c : label)
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-'))
      return false;
  return true;
}
bool known(const auto &items) {
  for (const auto &item : items)
    if (std::holds_alternative<ast::IncludeStmt *>(item))
      return false;
  return true;
}
bool whitespace(unsigned char c) {
  return c == ' ' || (c >= '\t' && c <= '\r');
}
} // namespace

void MetamathValidator::checkDatabase(
    const ast::Database &node, cursiv::ValidationAcceptor &accept) const {
  const auto *document = node.getDocument();
  if (!document)
    return;
  std::unordered_set<std::string_view> labels;
  std::vector<cursiv::TextRange> filenames;
  for (auto child : document->parseResult.nodes) {
    if (auto labeled = cursiv::ast_cast<ast::LabeledStatement>(child))
      if (!labeled->label.empty() && !labels.insert(labeled->label).second)
        accept
            .error(*labeled, "This label is already declared",
                   "metamath.duplicate-label")
            .property<&ast::LabeledStatement::label>();
    if (auto include = cursiv::ast_cast<ast::IncludeStmt>(child))
      if (auto range = accept.featureSpan(*include, "filename", 0))
        filenames.push_back(*range);
  }
  std::vector<const ast::ScopeOpen *> scopes;
  const cursiv::AstNode *previous = nullptr;
  for (const auto &statement : node.stmts) {
    std::visit(
        [&](const cursiv::AstNode *child) {
          if (auto labeled = cursiv::ast_cast<ast::LabeledStatement>(child))
            if (labeled->label.empty() &&
                !cursiv::ast_cast<ast::IncludeStmt>(previous))
              accept
                  .error(*labeled, "This statement needs a label",
                         "metamath.label")
                  .property<&ast::LabeledStatement::label>();
          if (filenames.empty()) {
            if (auto open = cursiv::ast_cast<ast::ScopeOpen>(child)) {
              scopes.push_back(open);
            } else if (auto close = cursiv::ast_cast<ast::ScopeClose>(child)) {
              if (scopes.empty())
                accept.error(*close, "This scope has no opening marker",
                             "metamath.scope-close");
              else
                scopes.pop_back();
            } else if (!scopes.empty() &&
                       cursiv::ast_cast<ast::ConstantStmt>(child)) {
              accept.error(*child,
                           "Constants must be declared in the outermost scope",
                           "metamath.constant-scope");
            }
          }
          previous = child;
        },
        statement);
  }
  for (auto open : scopes)
    accept.error(*open, "This scope has no closing marker",
                 "metamath.scope-open");
  const auto text = document->textDocument().getText();
  bool comment = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto c = static_cast<unsigned char>(text[i]);
    if (!(c >= 33 && c <= 126) && !whitespace(c))
      accept
          .error(node, "Only printable ASCII and ASCII whitespace are allowed",
                 "metamath.character")
          .range({i, i + 1});
    if (c != '$' || i + 1 == text.size())
      continue;
    bool filename = false;
    for (const auto range : filenames)
      if (range.begin <= i && i < range.end) {
        filename = true;
        break;
      }
    if (filename)
      continue;
    if ((!comment || text[i + 1] == ')') &&
        ((i && !whitespace(static_cast<unsigned char>(text[i - 1]))) ||
         (i + 2 < text.size() &&
          !whitespace(static_cast<unsigned char>(text[i + 2])))))
      accept
          .error(node, "A keyword must be separated by whitespace",
                 "metamath.keyword-space")
          .range({i, i + 2});
    if (text[i + 1] == '(') {
      if (comment)
        accept
            .error(node, "Comments cannot be nested",
                   "metamath.comment-nesting")
            .range({i, i + 2});
      comment = true;
    } else if (text[i + 1] == ')') {
      comment = false;
    }
  }
}

void MetamathValidator::checkMath(const ast::MathStatement &node,
                                  cursiv::ValidationAcceptor &accept) const {
  const auto *labeled = cursiv::ast_cast<ast::LabeledStatement>(&node);
  if (labeled && !labeled->label.empty() && !validLabel(labeled->label))
    accept
        .error(*labeled,
               "A label must contain letters, digits, '.', '_' or '-'",
               "metamath.label")
        .property<&ast::LabeledStatement::label>();
  if (!known(node.math))
    return;
  if (cursiv::ast_cast<ast::FloatingStmt>(&node)) {
    if (node.math.size() != 2)
      accept.error(node, "A floating hypothesis needs exactly two math symbols",
                   "metamath.floating-size");
  } else if (cursiv::ast_cast<ast::DisjointStmt>(&node)) {
    if (node.math.size() < 2)
      accept.error(node, "A disjoint declaration needs at least two variables",
                   "metamath.disjoint-size");
  } else if (node.math.empty()) {
    accept.error(node, "This statement needs at least one math symbol",
                 "metamath.math-empty");
  }
}

void MetamathValidator::checkStep(const ast::ProofStep &node,
                                  cursiv::ValidationAcceptor &accept) const {
  if ((node.label != "?" && !validLabel(node.label)) ||
      (!node.local.empty() && !validLabel(node.local)) ||
      (!node.target.empty() && !validLabel(node.target)))
    accept.error(node,
                 "A proof label must contain letters, digits, '.', '_' or '-'",
                 "metamath.proof-label");
}

void MetamathValidator::checkPlain(const ast::UncompressedProof &node,
                                   cursiv::ValidationAcceptor &accept) const {
  if (node.steps.empty())
    accept.error(node, "An empty proof must be written as '?'",
                 "metamath.proof-empty");
  for (std::size_t i = 0; i < node.steps.size(); ++i)
    if (auto text = std::get_if<std::string_view>(&node.steps[i]);
        text && *text != "?" && !validLabel(*text))
      accept
          .error(node,
                 "A proof label must contain letters, digits, '.', '_' or '-'",
                 "metamath.proof-label")
          .property<&ast::UncompressedProof::steps>(i);
}

void MetamathValidator::checkCompressed(
    const ast::CompressedProof &node,
    cursiv::ValidationAcceptor &accept) const {
  for (std::size_t i = 0; i < node.labels.size(); ++i)
    if (auto text = std::get_if<std::string_view>(&node.labels[i]);
        text && !validLabel(*text))
      accept
          .error(node,
                 "A proof label must contain letters, digits, '.', '_' or '-'",
                 "metamath.proof-label")
          .property<&ast::CompressedProof::labels>(i);
  if (node.blocks.empty())
    accept.error(node, "An empty proof must be written as '?'",
                 "metamath.proof-empty");
  for (std::size_t i = 0; i < node.blocks.size(); ++i)
    if (auto text = std::get_if<std::string_view>(&node.blocks[i]))
      for (char c : *text)
        if (!(c >= 'A' && c <= 'Z') && c != '?') {
          accept
              .error(node, "Compressed proof data may contain only A-Z and '?'",
                     "metamath.compressed-code")
              .property<&ast::CompressedProof::blocks>(i);
          break;
        }
}

void register_validation_checks(MetamathCoreServices &services) {
  services.validator = std::make_unique<const MetamathValidator>(services);
  using V = MetamathValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDatabase>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMath>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkStep>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPlain>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCompressed>(
           *services.validator)});
}

} // namespace metamath
