#include "v/core/validation/VValidator.hpp"

#include "v/core/CoreServices.hpp"
#include "cursiv/core/syntax-tree/AstReflection.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

#include <optional>
#include <vector>

namespace v {
namespace {
// The 0.4.12 scanner only ends this exceptional hole on a final name.
// A data-rule feature anchor can include adopted parentheses; the borrowed
// scalar spelling keeps the exact name extent without inspecting source text.
std::optional<cursiv::TextRange> nameSpan(
    const cursiv::AstNode& node, std::string_view feature,
    std::string_view spelling, cursiv::ValidationAcceptor& accept) {
  const auto anchor = accept.featureSpan(node, feature, 0);
  const auto name = accept.spanOfName(spelling, node);
  if (spelling.empty() || !anchor || name.begin < anchor->begin ||
      name.end > anchor->end || name.end - name.begin != spelling.size())
    return {};
  return name;
}

std::optional<cursiv::TextRange> finalName(
    const ast::Expr* expression, cursiv::ValidationAcceptor& accept) {
  if (!expression) return {};
  if (const auto* name = cursiv::ast_cast<ast::Ident>(expression))
    return nameSpan(*name, "name", name->name, accept);
  if (const auto* selector = cursiv::ast_cast<ast::SelectorExpr>(expression))
    return nameSpan(*selector, "field", selector->field, accept);
  if (const auto* value = cursiv::ast_cast<ast::EnumValue>(expression))
    return nameSpan(*value, "name", value->name, accept);
  if (const auto* prefix = cursiv::ast_cast<ast::PrefixExpr>(expression))
    return finalName(prefix->operand, accept);
  if (const auto* binary = cursiv::ast_cast<ast::BinaryExpr>(expression))
    return finalName(binary->right, accept);
  if (const auto* range = cursiv::ast_cast<ast::RangeExpr>(expression))
    return finalName(range->high, accept);
  return {};
}

bool containsString(const ast::Expr* expression,
                    const cursiv::AstReflection& reflection) {
  std::vector<const cursiv::AstNode*> pending;
  if (expression) pending.push_back(expression);
  while (!pending.empty()) {
    const auto* node = pending.back();
    pending.pop_back();
    if (cursiv::ast_cast<ast::StringLit>(node) ||
        cursiv::ast_cast<ast::InterpolatedString>(node))
      return true;
    reflection.visitMembers(*node, [&](std::string_view, std::uint32_t,
                                       cursiv::AstNode& child) {
      pending.push_back(&child);
    });
  }
  return false;
}

// A name continues only into an adjacent selector or call. A closing call
// continues only into a selector, as in `$f().field`.
bool continues(const cursiv::AstNode& node, cursiv::TextOffset end,
               bool allowCall, cursiv::ValidationAcceptor& accept) {
  const auto* parent = node.parent;
  std::string_view punctuation;
  if (const auto* selector = cursiv::ast_cast<ast::SelectorExpr>(parent);
      selector && selector->object == &node)
    punctuation = ".";
  else if (const auto* call = cursiv::ast_cast<ast::CallExpr>(parent);
           allowCall && call && call->callee == &node)
    punctuation = "(";
  else
    return false;
  const auto anchor = accept.keywordSpan(*parent, punctuation, 0);
  return anchor && anchor->begin == end && anchor->end == end + 1;
}

bool invalidShortArguments(const ast::Expr* expression,
                           const cursiv::AstReflection& reflection,
                           cursiv::ValidationAcceptor& accept) {
  struct Pending { const cursiv::AstNode* node; bool argument; };
  std::vector<Pending> pending;
  if (expression) pending.push_back({expression, false});
  while (!pending.empty()) {
    const auto [node, argument] = pending.back();
    pending.pop_back();
    const auto* call = cursiv::ast_cast<ast::CallExpr>(node);
    if (argument && reflection.isInstance(*node, &cursiv::ast_type_tag<ast::Expr>)) {
      const auto extent = accept.spanOf(*node);
      const auto open = accept.keywordSpan(*node, "(", 0);
      // An absent keyword falls back to the whole node, including for `1`.
      // A genuine adopted '(' is a strict one-byte prefix of its expression.
      if (open && open->begin == extent.begin && open->end == open->begin + 1 &&
          open->end < extent.end)
        return true;
      if (cursiv::ast_cast<ast::StringLit>(node) ||
          cursiv::ast_cast<ast::InterpolatedString>(node))
        return true;
      if (const auto* name = cursiv::ast_cast<ast::Ident>(node)) {
        const auto span = nameSpan(*name, "name", name->name, accept);
        if (!span || !continues(*node, span->end, true, accept)) return true;
      }
      if (const auto* selector = cursiv::ast_cast<ast::SelectorExpr>(node)) {
        const auto span = nameSpan(*selector, "field", selector->field, accept);
        if (!span || !continues(*node, span->end, true, accept)) return true;
      }
      if (call && !continues(*node, extent.end, false, accept)) return true;
    }
    reflection.visitMembers(*node, [&](std::string_view member, std::uint32_t,
                                       cursiv::AstNode& child) {
      pending.push_back({&child, argument || (call && member == "args")});
    });
  }
  return false;
}
}  // namespace

void VValidator::checkInterpolation(
    const ast::StringInterpolation& interpolation,
    cursiv::ValidationAcceptor& accept) const {
  if (!interpolation.braced) {
    if (invalidShortArguments(interpolation.expression, *services.astReflection, accept))
      accept.error(interpolation, "Use '${...}' around this interpolation call.",
                   "v.interpolation-short-argument");
    return;
  }
  if (interpolation.hasClosingBrace) return;
  std::optional<cursiv::TextRange> last;
  if (interpolation.format) {
    if (!interpolation.format->code.empty())
      last = nameSpan(*interpolation.format, "code", interpolation.format->code, accept);
  } else {
    last = finalName(interpolation.expression, accept);
  }
  if (!last || last->end != accept.spanOf(interpolation).end ||
      containsString(interpolation.expression, *services.astReflection))
    accept.error(interpolation, "Expected '}' before the closing quote.",
                 "v.interpolation-close");
}

void VValidator::checkUnsafe(const ast::UnsafeExpr& expression,
                            cursiv::ValidationAcceptor& accept) const {
  for (const auto* parent = expression.parent; parent; parent = parent->parent)
    if (cursiv::ast_cast<ast::UnsafeExpr>(parent)) {
      accept.error(expression, "Already inside an unsafe expression.", "v.unsafe-nested");
      return;
    }
}

void register_validation_checks(VCoreServices& services) {
  services.validator = std::make_unique<const VValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &VValidator::checkInterpolation>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &VValidator::checkUnsafe>(*services.validator)});
}

}  // namespace v
