#include "javascript/core/validation/JavaScriptValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/syntax-tree/AstNode.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "javascript/core/CoreServices.hpp"

namespace javascript {

namespace {

// A directive prologue opening with "use strict", in either quote.
template <class Body>
bool prologue_strict(const Body& body) {
  for (const ast::Stmt* const stmt : body) {
    const auto* const expr = cursiv::ast_cast<ast::ExprStmt>(stmt);
    if (expr == nullptr) return false;
    const auto* const lit = cursiv::ast_cast<ast::StringLit>(expr->expression);
    if (lit == nullptr) return false;
    if (lit->text == "\"use strict\"" || lit->text == "'use strict'")
      return true;
  }
  return false;
}

// Module code is strict throughout.
bool is_module(const ast::Program& program) {
  for (const ast::Stmt* const stmt : program.body)
    if (cursiv::ast_cast<ast::ImportStmt>(stmt) != nullptr ||
        cursiv::ast_cast<ast::ExportStmt>(stmt) != nullptr)
      return true;
  return false;
}

// Whether the code around `node` is strict: its own function's prologue, a
// class body, or the program's prologue, whichever encloses it first says
// so; a sloppy function inside a strict one inherits.
bool is_strict(const cursiv::AstNode* node) {
  for (const cursiv::AstNode* p = node->parent; p != nullptr; p = p->parent) {
    if (const auto* const f = cursiv::ast_cast<ast::FunctionDecl>(p)) {
      if (f->body != nullptr && prologue_strict(f->body->body)) return true;
    } else if (const auto* const f = cursiv::ast_cast<ast::FunctionExpr>(p)) {
      if (f->body != nullptr && prologue_strict(f->body->body)) return true;
    } else if (const auto* const f = cursiv::ast_cast<ast::ArrowFunctionExpr>(p)) {
      if (f->block != nullptr && prologue_strict(f->block->body)) return true;
    } else if (cursiv::ast_cast<ast::ClassDecl>(p) != nullptr ||
               cursiv::ast_cast<ast::ClassExpr>(p) != nullptr) {
      return true;
    } else if (const auto* const program = cursiv::ast_cast<ast::Program>(p)) {
      return is_module(*program) || prologue_strict(program->body);
    }
  }
  return false;
}

bool strict_only(std::string_view name) {
  for (const std::string_view w : {"implements", "let", "private", "public",
                                   "interface", "package", "protected",
                                   "static"})
    if (name == w) return true;
  return false;
}

// A property name is any word: `o.static`, `{let: 1}`, `class { static(){} }`.
bool is_property_name(const ast::Ident& ident) {
  const cursiv::AstNode* const p = ident.parent;
  if (const auto* const m = cursiv::ast_cast<ast::MemberExpr>(p))
    return !m->computed && m->property == &ident;
  if (const auto* const prop = cursiv::ast_cast<ast::Prop>(p))
    return !prop->computed && prop->key == &ident;
  if (const auto* const member = cursiv::ast_cast<ast::ClassMember>(p))
    return !member->computed && member->key == &ident;
  return false;
}

bool legacy_octal(std::string_view text) {
  if (text.size() < 2 || text[0] != '0') return false;
  for (const char c : text.substr(1))
    if (c < '0' || c > '7') return false;
  return true;
}

}  // namespace

void JavaScriptValidator::checkStrictName(
    const ast::Ident& ident, cursiv::ValidationAcceptor& accept) const {
  if (!strict_only(ident.name) || is_property_name(ident) || !is_strict(&ident))
    return;
  accept.error(ident, "'" + std::string(ident.name) +
                          "' is reserved in strict mode");
}

void JavaScriptValidator::checkLegacyOctal(
    const ast::NumberLit& lit, cursiv::ValidationAcceptor& accept) const {
  if (!legacy_octal(lit.text) || !is_strict(&lit)) return;
  accept.error(lit, "legacy octal literals are not allowed in strict mode");
}

void register_validation_checks(JavaScriptCoreServices& services) {
  using V = JavaScriptValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkStrictName>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLegacyOctal>(
           validator)});
}

}  // namespace javascript
