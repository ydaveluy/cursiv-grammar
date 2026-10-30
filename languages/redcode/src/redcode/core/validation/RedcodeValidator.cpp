#include "redcode/core/validation/RedcodeValidator.hpp"

#include <memory>
#include <charconv>
#include <limits>
#include <optional>
#include "cursiv/core/validation/ValidationRegistry.hpp"

#include "redcode/core/CoreServices.hpp"

namespace redcode {
namespace {
bool active(const cursiv::AstNode &node) {
  const ast::Instruction *line = nullptr;
  for (const auto *p = &node; p && !line; p = p->parent)
    line = cursiv::ast_cast<ast::Instruction>(p);
  const auto *file = line ? cursiv::ast_cast<ast::File>(line->parent) : nullptr;
  if (!file) return true;
  for (const auto &item : file->lines)
    if (const auto *p = std::get_if<ast::Instruction*>(&item); p && *p) {
      if (*p == line) return true;
      if ((*p)->opcode == ast::Opcode::end) return false;
    }
  return true;
}
using I = std::int64_t;
constexpr I low = std::numeric_limits<I>::min(), high = std::numeric_limits<I>::max();
std::optional<I> constant(const ast::Expr *node) {
  if (const auto *n = cursiv::ast_cast<ast::Number>(node)) {
    std::uint64_t v = 0;
    const auto [end, ec] = std::from_chars(n->magnitude.data(), n->magnitude.data() + n->magnitude.size(), v);
    if (ec != std::errc{} || end != n->magnitude.data() + n->magnitude.size()) return {};
    if (n->negative && v == std::uint64_t(high) + 1) return low;
    if (v > std::uint64_t(high)) return {};
    return n->negative ? -static_cast<I>(v) : static_cast<I>(v);
  }
  if (const auto *p = cursiv::ast_cast<ast::Paren>(node)) return constant(p->inner);
  if (const auto *u = cursiv::ast_cast<ast::Unary>(node)) {
    const auto v = constant(u->operand); if (!v) return {};
    if (u->op == ast::Op::logical_not) return !*v;
    if (u->op == ast::Op::add) return *v;
    if (*v != low) return -*v;
    return {};
  }
  const auto *b = cursiv::ast_cast<ast::BinaryExpr>(node);
  if (!b) return {};
  const auto left = constant(b->left), right = constant(b->right);
  if (!left || !right) return {};
  const I a = *left, c = *right;
  switch (b->op) {
    case ast::Op::add:
      if ((c > 0 && a > high - c) || (c < 0 && a < low - c)) return {};
      return a + c;
    case ast::Op::sub:
      if ((c > 0 && a < low + c) || (c < 0 && a > high + c)) return {};
      return a - c;
    case ast::Op::mul:
      if (!a || !c) return 0;
      if (a > 0 ? (c > 0 ? a > high / c : c < low / a)
                : (c > 0 ? a < low / c : a < high / c)) return {};
      return a * c;
    case ast::Op::div: case ast::Op::mod:
      if (!c || (a == low && c == -1)) return {};
      return b->op == ast::Op::div ? a / c : a % c;
    case ast::Op::eq: return a == c;
    case ast::Op::ne: return a != c;
    case ast::Op::lt: return a < c;
    case ast::Op::le: return a <= c;
    case ast::Op::gt: return a > c;
    case ast::Op::ge: return a >= c;
    case ast::Op::logical_and: return a && c;
    case ast::Op::logical_or: return a || c;
    default: return {};
  }
}
}  // namespace

void RedcodeValidator::checkInstruction(const ast::Instruction &node, cursiv::ValidationAcceptor &accept) const {
  if (!active(node)) return;
  using O = ast::Opcode;
  if (node.opcode == O::org || node.opcode == O::end) return;
  if (!node.field_b && node.opcode != O::dat && node.opcode != O::jmp &&
      node.opcode != O::spl && node.opcode != O::nop)
    accept.error(node, "This instruction needs two operands.", "redcode.operands");
}
void RedcodeValidator::checkBinary(const ast::BinaryExpr &node, cursiv::ValidationAcceptor &accept) const {
  if (!active(node)) return;
  if (node.op != ast::Op::div && node.op != ast::Op::mod) return;
  const auto denominator = constant(node.right);
  if (denominator && *denominator == 0)
    accept.error(node, "Division by zero in operand expression.", "redcode.expression");
}

void register_validation_checks(RedcodeCoreServices& services) {
  services.validator = std::make_unique<const RedcodeValidator>(services);
  using V = RedcodeValidator;
  const auto &v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInstruction>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBinary>(v)});
}

}  // namespace redcode
