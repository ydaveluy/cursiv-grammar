#include "r/core/validation/RValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "r/core/CoreServices.hpp"
#include "cursiv/core/workspace/Document.hpp"

namespace r {
namespace {
const ast::Placeholder *extractionBase(const ast::Expr *node) {
  if (const auto *p = cursiv::ast_cast<ast::Placeholder>(node)) return p;
  if (const auto *p = cursiv::ast_cast<ast::IndexExpr>(node)) return extractionBase(p->object);
  if (const auto *p = cursiv::ast_cast<ast::BinaryExpr>(node); p && (p->op == "$" || p->op == "@"))
    return extractionBase(p->left);
  return nullptr;
}
}  // namespace

void RValidator::checkFunction(const ast::FunctionDef &node, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto *p : node.params) {
    const auto name = p->dots ? "..." : p->dot ? "." : p->name;
    if (!names.insert(name).second) accept.error(*p, "A formal argument cannot be repeated.", "r.formal");
  }
}
void RValidator::checkBinary(const ast::BinaryExpr &node, cursiv::ValidationAcceptor &accept) const {
  if (node.op == "::" || node.op == ":::") {
    if (!cursiv::ast_cast<ast::Ident>(node.left) && !cursiv::ast_cast<ast::StringLit>(node.left))
      accept.error(node, "A namespace qualifier needs a name or string.", "r.namespace");
  } else if (node.op == "|>") {
    const auto *bind = cursiv::ast_cast<ast::BinaryExpr>(node.right);
    if (bind && bind->op == "=>" && cursiv::ast_cast<ast::Ident>(bind->left)) return;
    const auto *call = cursiv::ast_cast<ast::CallExpr>(node.right);
    if (!call && (!extractionBase(node.right) || cursiv::ast_cast<ast::Placeholder>(node.right)))
      accept.error(node, "A pipe needs a function call or placeholder extraction as its right operand.", "r.pipe");
    if (call) {
      unsigned placeholders = 0;
      for (const auto *a : call->args)
        placeholders += cursiv::ast_cast<ast::Placeholder>(a->value) != nullptr;
      if (placeholders > 1) accept.error(node, "A pipe placeholder may only appear once.", "r.pipe");
    }
  } else if (node.op == "=>") {
    const auto *pipe = cursiv::ast_cast<ast::BinaryExpr>(node.parent);
    if (!pipe || pipe->op != "|>" || pipe->right != &node ||
        !cursiv::ast_cast<ast::Ident>(node.left))
      accept.error(node, "A pipe bind needs a symbol on the right side of a pipe.", "r.pipebind");
  }
}
void RValidator::checkPlaceholder(const ast::Placeholder &node, cursiv::ValidationAcceptor &accept) const {
  const cursiv::AstNode *child = &node;
  for (const auto *parent = node.parent; parent; child = parent, parent = parent->parent) {
    const auto *pipe = cursiv::ast_cast<ast::BinaryExpr>(parent);
    if (!pipe || pipe->op != "|>" || pipe->right != child) continue;
    if (pipe->right != &node && extractionBase(pipe->right) == &node) return;
    if (const auto *arg = cursiv::ast_cast<ast::Arg>(node.parent);
        arg && !arg->name.empty() && arg->value == &node && arg->parent == pipe->right) return;
    break;
  }
  accept.error(node, "Invalid pipe placeholder context.", "r.placeholder");
}
void RValidator::checkAssignment(const ast::AssignExpr &node, cursiv::ValidationAcceptor &accept) const {
  if (node.op != "=" || !node.parent) return;
  const auto *p = node.parent;
  if (cursiv::ast_cast<ast::Program>(p) || cursiv::ast_cast<ast::Block>(p) ||
      cursiv::ast_cast<ast::ParenExpr>(p) || cursiv::ast_cast<ast::FunctionDef>(p) ||
      cursiv::ast_cast<ast::HelpExpr>(p) || cursiv::ast_cast<ast::RepeatExpr>(p)) return;
  if (const auto *a = cursiv::ast_cast<ast::AssignExpr>(p); a && a->op == "=") return;
  if (const auto *a = cursiv::ast_cast<ast::BinaryExpr>(p); a && a->op == "?") return;
  if (const auto *a = cursiv::ast_cast<ast::IfExpr>(p); a && a->cond != &node) return;
  if (const auto *a = cursiv::ast_cast<ast::WhileExpr>(p); a && a->body == &node) return;
  if (const auto *a = cursiv::ast_cast<ast::ForExpr>(p); a && a->body == &node) return;
  accept.error(node, "Parenthesize = assignment in this expression context.", "r.assignment");
}
void RValidator::checkIf(const ast::IfExpr &node, cursiv::ValidationAcceptor &accept) const {
  if (!node.thenBranch || !node.elseBranch) return;
  for (const auto *p = node.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::Block>(p) || cursiv::ast_cast<ast::ParenExpr>(p) ||
        cursiv::ast_cast<ast::CallExpr>(p) || cursiv::ast_cast<ast::IndexExpr>(p)) return;
  const auto *document = node.getDocument();
  const auto key = accept.keywordSpan(node, "else", 0);
  if (!document || !key) return;
  const auto begin = accept.spanOf(*node.thenBranch).end;
  const auto source = document->textDocument().getText();
  if (begin <= key->begin && key->begin <= source.size() &&
      source.substr(begin, key->begin - begin).find_first_of("\r\n") != std::string_view::npos)
    accept.error(node, "A top-level else must continue the same logical line.", "r.else");
}

void register_validation_checks(RCoreServices& services) {
  services.validator = std::make_unique<const RValidator>(services);
  using V = RValidator; const auto &v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBinary>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPlaceholder>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkIf>(v)});
}

}  // namespace r
