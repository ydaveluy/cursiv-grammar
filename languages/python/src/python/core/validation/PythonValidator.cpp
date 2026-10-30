#include "python/core/validation/PythonValidator.hpp"

#include <memory>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "python/core/CoreServices.hpp"

namespace python {

namespace {

bool singleTarget(const ast::Expr *e) {
  return cursiv::ast_cast<ast::Name>(e) || cursiv::ast_cast<ast::Attribute>(e) ||
         cursiv::ast_cast<ast::Subscript>(e);
}

bool unpackTarget(const ast::Expr *e, bool deleting = false) {
  if (singleTarget(e)) return true;
  const auto sequence = [&](const auto &items) {
    unsigned stars = 0;
    for (const auto *item : items) {
      if (const auto *star = cursiv::ast_cast<ast::StarredExpr>(item)) {
        if (deleting || ++stars > 1 || !unpackTarget(star->value)) return false;
      } else if (!unpackTarget(item, deleting)) return false;
    }
    return true;
  };
  if (const auto *tuple = cursiv::ast_cast<ast::TupleExpr>(e)) return sequence(tuple->elts);
  if (const auto *list = cursiv::ast_cast<ast::ListExpr>(e)) return sequence(list->elts);
  return false;
}

bool byteString(std::string_view text) {
  const auto end = text.find_first_of("\"'");
  return text.substr(0, end).find_first_of("bB") != std::string_view::npos;
}

bool validString(std::string_view text) {
  const auto start = text.find_first_of("\"'");
  if (start == std::string_view::npos) return true;
  const auto prefix = text.substr(0, start);
  const auto quotes = text.substr(start).starts_with("\"\"\"") ||
                      text.substr(start).starts_with("'''") ? 3u : 1u;
  const auto body = text.substr(start + quotes, text.size() - start - 2 * quotes);
  const bool bytes = byteString(text), raw = prefix.find_first_of("rR") != std::string_view::npos;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (bytes && static_cast<unsigned char>(body[i]) >= 128) return false;
    if (raw || body[i] != '\\' || i + 1 == body.size()) continue;
    const auto e = body[++i];
    const unsigned count = e == 'x' ? 2 : !bytes && e == 'u' ? 4 : !bytes && e == 'U' ? 8 : 0;
    if (count) {
      if (body.size() - i - 1 < count) return false;
      unsigned value = 0;
      for (unsigned j = 0; j < count; ++j) {
        const unsigned char c = body[++i];
        const unsigned d = c >= '0' && c <= '9' ? c - '0' :
                           c >= 'a' && c <= 'f' ? c - 'a' + 10 :
                           c >= 'A' && c <= 'F' ? c - 'A' + 10 : 16;
        if (d == 16) return false;
        value = value * 16 + d;
      }
      // CPython string values can contain surrogate code points.
      if (count == 8 && value > 0x10ffff) return false;
    } else if (!bytes && e == 'N') {
      if (i + 1 == body.size() || body[++i] != '{') return false;
      const auto end = body.find('}', i + 1);
      if (end == std::string_view::npos || end == i + 1) return false;
      i = end; // Named-character lookup is outside this small validator.
    }
  }
  return true;
}

}  // namespace

void PythonValidator::checkParams(const ast::Params &node, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> names;
  bool defaultSeen = false;
  const auto check = [&](const ast::Param *p, bool positional, bool vararg = false) {
    if (!p) return;
    if (!names.insert(p->name).second)
      accept.error(*p, "A parameter name must be unique.", "python.parameter");
    if (positional) {
      if (defaultSeen && !p->defaultValue)
        accept.error(*p, "A required positional parameter cannot follow a default.", "python.parameter");
      defaultSeen |= p->defaultValue != nullptr;
    }
    if (!vararg && cursiv::ast_cast<ast::StarredExpr>(p->annotation))
      accept.error(*p, "A starred annotation belongs to the var-positional parameter.", "python.parameter");
  };
  for (const auto *p : node.posonly) check(p, true);
  for (const auto *p : node.args) check(p, true);
  check(node.vararg, false, true);
  for (const auto *p : node.kwonly) check(p, false);
  check(node.kwarg, false);
  for (const auto *p : {node.vararg, node.kwarg})
    if (p && p->defaultValue)
      accept.error(*p, "A variadic parameter cannot have a default.", "python.parameter");
}

void PythonValidator::checkStr(const ast::Str &node, cursiv::ValidationAcceptor &accept) const {
  if (!validString(node.text)) accept.error(node, "Invalid Python string escape or non-ASCII bytes literal.", "python.string");
}

void PythonValidator::checkStrings(const ast::Strings &node, cursiv::ValidationAcceptor &accept) const {
  bool bytes = false, text = false;
  for (const auto &piece : node.pieces) {
    if (const auto *p = std::get_if<ast::Str*>(&piece); p && *p) {
      bytes |= byteString((*p)->text);
      text |= !byteString((*p)->text);
    } else text = true;
  }
  if (bytes && text) accept.error(node, "Bytes and text literals cannot be concatenated.", "python.string");
}

void PythonValidator::checkAssign(const ast::AssignStmt &node, cursiv::ValidationAcceptor &accept) const {
  for (const auto *target : node.targets)
    if (!unpackTarget(target)) accept.error(node, "Invalid assignment target.", "python.target");
}
void PythonValidator::checkAugAssign(const ast::AugAssignStmt &node, cursiv::ValidationAcceptor &accept) const {
  if (!singleTarget(node.target)) accept.error(node, "Augmented assignment needs a single target.", "python.target");
}
void PythonValidator::checkAnnAssign(const ast::AnnAssignStmt &node, cursiv::ValidationAcceptor &accept) const {
  if (!singleTarget(node.target)) accept.error(node, "An annotated assignment needs a single target.", "python.target");
}
void PythonValidator::checkDel(const ast::DelStmt &node, cursiv::ValidationAcceptor &accept) const {
  for (const auto *target : node.targets)
    if (!unpackTarget(target, true)) accept.error(node, "Invalid deletion target.", "python.target");
}
void PythonValidator::checkFor(const ast::ForStmt &node, cursiv::ValidationAcceptor &accept) const {
  if (!unpackTarget(node.target)) accept.error(node, "Invalid for-loop target.", "python.target");
}
void PythonValidator::checkComprehension(const ast::Comprehension &node, cursiv::ValidationAcceptor &accept) const {
  if (!unpackTarget(node.target)) accept.error(node, "Invalid comprehension target.", "python.target");
}
void PythonValidator::checkWithItem(const ast::WithItem &node, cursiv::ValidationAcceptor &accept) const {
  if (node.optionalVars && !unpackTarget(node.optionalVars)) accept.error(node, "Invalid with-statement target.", "python.target");
}
void PythonValidator::checkTry(const ast::TryStmt &node, cursiv::ValidationAcceptor &accept) const {
  if (node.handlers.empty()) return;
  for (std::size_t i = 0; i < node.handlers.size(); ++i) {
    const auto *h = node.handlers[i];
    if (h->isStar != node.handlers[0]->isStar)
      accept.error(*h, "except and except* cannot share one try statement.", "python.except");
    if (!h->type && (h->isStar || i + 1 != node.handlers.size()))
      accept.error(*h, "A bare except must be last and cannot use *.", "python.except");
  }
}
void PythonValidator::checkCall(const ast::CallExpr &node, cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto *kw : node.keywords)
    if (!kw->arg.empty() && !names.insert(kw->arg).second)
      accept.error(*kw, "A keyword argument cannot be repeated.", "python.argument");
}
void PythonValidator::checkExprStmt(const ast::ExprStmt &node, cursiv::ValidationAcceptor &accept) const {
  if (cursiv::ast_cast<ast::StarredExpr>(node.value))
    accept.error(node, "A starred expression needs a containing expression list.", "python.starred");
}
void PythonValidator::checkMatchValue(const ast::MatchValue &node, cursiv::ValidationAcceptor &accept) const {
  if (const auto *strings = cursiv::ast_cast<ast::Strings>(node.value))
    for (const auto &piece : strings->pieces)
      if (std::holds_alternative<ast::IStr*>(piece)) {
        accept.error(node, "A literal pattern cannot use an f-string or t-string.", "python.pattern");
        break;
      }
}

void register_validation_checks(PythonCoreServices& services) {
  services.validator = std::make_unique<const PythonValidator>(services);
  using V = PythonValidator;
  const V &v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkParams>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkStr>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkStrings>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssign>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAugAssign>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAnnAssign>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkDel>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFor>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkComprehension>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkWithItem>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkTry>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkExprStmt>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkMatchValue>(v)});
}

}  // namespace python
