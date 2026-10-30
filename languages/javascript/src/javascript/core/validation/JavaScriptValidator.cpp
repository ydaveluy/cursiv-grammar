#include "javascript/core/validation/JavaScriptValidator.hpp"

#include <memory>
#include <string>
#include "cursiv/core/syntax-tree/AstReflection.hpp"
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
                                   "static", "yield"})
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
  return text.size() > 1 && text[0] == '0' && text[1] >= '0' && text[1] <= '9';
}

bool hard_reserved(std::string_view name) {
  constexpr std::string_view words = " break do instanceof typeof case else new var catch finally return void continue for switch while debugger function this with default if throw delete in try class enum extends super const export import null true false ";
  return !name.empty() && words.find(" " + std::string(name) + " ") != words.npos;
}

bool async_or_generator(const cursiv::AstNode& node, bool async) {
  for (const auto* p = node.parent; p; p = p->parent) {
    if (const auto* f = cursiv::ast_cast<ast::FunctionDecl>(p))
      return async ? f->isAsync : f->isGenerator;
    if (const auto* f = cursiv::ast_cast<ast::FunctionExpr>(p)) {
      bool value = async ? f->isAsync : f->isGenerator;
      if (const auto* owner = cursiv::ast_cast<ast::Prop>(f->parent)) value |= async ? owner->isAsync : owner->isGenerator;
      if (const auto* owner = cursiv::ast_cast<ast::ClassMember>(f->parent)) value |= async ? owner->isAsync : owner->isGenerator;
      return value;
    }
    if (const auto* f = cursiv::ast_cast<ast::ArrowFunctionExpr>(p)) return async && f->isAsync;
  }
  return false;
}

struct EscapeState { bool invalid = false; bool legacy = false; };
bool hexadecimal(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
EscapeState escapes(std::string_view body) {
  EscapeState state;
  for (std::size_t i = 0; i < body.size(); ++i) {
    if (body[i] != '\\') continue;
    if (++i == body.size()) { state.invalid = true; break; }
    const char c = body[i];
    if (c >= '1' && c <= '9') state.legacy = true;
    if (c == '0' && i + 1 < body.size() && body[i + 1] >= '0' && body[i + 1] <= '9') state.legacy = true;
    if (c != 'u' && c != 'x') continue;
    const bool braced = c == 'u' && i + 1 < body.size() && body[i + 1] == '{';
    if (braced) ++i;
    const unsigned limit = c == 'x' ? 2u : 4u;
    unsigned count = 0, value = 0;
    while (i + 1 < body.size() && (braced ? body[i + 1] != '}' : count < limit)) {
      const char h = body[++i];
      if (!hexadecimal(h)) { state.invalid = true; break; }
      const unsigned digit = h <= '9' ? unsigned(h - '0') : unsigned((h | 32) - 'a' + 10);
      if (value > (0x10ffffu - digit) / 16) { state.invalid = true; break; }
      value = value * 16 + digit; ++count;
    }
    if (!count || (braced ? i + 1 == body.size() || body[i + 1] != '}' : count != limit)) state.invalid = true;
    if (braced && i + 1 < body.size() && body[i + 1] == '}') ++i;
  }
  return state;
}

template<class Params>
void parameters(const Params& list, const ast::BlockStmt* body, cursiv::ValidationAcceptor& accept) {
  bool simple = true;
  for (std::size_t i = 0; i < list.size(); ++i) {
    const auto* p = list[i]; if (!p) continue;
    simple &= !p->rest && !p->defaultValue && cursiv::ast_cast<ast::Ident>(p->pattern);
    if (p->rest && (i + 1 != list.size() || p->defaultValue ||
        (!cursiv::ast_cast<ast::Ident>(p->pattern) && !cursiv::ast_cast<ast::ArrayLit>(p->pattern) && !cursiv::ast_cast<ast::ObjectLit>(p->pattern))))
      accept.error(*p, "A rest parameter must be a final binding without a default.", "javascript.rest-parameter");
  }
  if (!simple && body && prologue_strict(body->body))
    accept.error(*body, "A function with non-simple parameters cannot contain a use strict directive.", "javascript.strict-parameters");
}

void accessor(bool get, bool set, const ast::Expr* value, const cursiv::AstNode& owner, cursiv::ValidationAcceptor& accept) {
  const auto* f = cursiv::ast_cast<ast::FunctionExpr>(value);
  if (!f) return;
  if ((get && !f->params.empty()) || (set && (f->params.size() != 1 || f->params[0]->rest)))
    accept.error(owner, "A getter has no parameters; a setter has one non-rest parameter.", "javascript.accessor-parameters");
}
std::string_view property_name(const ast::Expr* key) {
  if (const auto* id = cursiv::ast_cast<ast::Ident>(key)) return id->name;
  if (const auto* string = cursiv::ast_cast<ast::StringLit>(key); string && string->text.size() >= 2 && string->text.find('\\') == string->text.npos)
    return string->text.substr(1, string->text.size() - 2);
  return {};
}

}  // namespace

void JavaScriptValidator::checkLegacyOctal(
    const ast::NumberLit& lit, cursiv::ValidationAcceptor& accept) const {
  if (!legacy_octal(lit.text) || !is_strict(&lit)) return;
  accept.error(lit, "legacy octal literals are not allowed in strict mode");
}

void JavaScriptValidator::checkNames(const ast::Program& program, cursiv::ValidationAcceptor& accept) const {
  const bool module = is_module(program);
  const auto name = [&](const cursiv::AstNode& node, std::string_view text, bool strict, bool async = false, bool generator = false) {
    if (text.empty()) return;
    if (hard_reserved(text) || (strict && strict_only(text)) ||
        (text == "await" && (module || async || async_or_generator(node, true))) ||
        (text == "yield" && (generator || async_or_generator(node, false))))
      accept.error(node, "A reserved word cannot be used as this name.", "javascript.reserved-name");
  };
  const auto walk = [&](auto&& self, const cursiv::AstNode& node) -> void {
    if (const auto* n = cursiv::ast_cast<ast::Ident>(&node); n && !is_property_name(*n)) name(node, n->name, is_strict(n));
    if (const auto* n = cursiv::ast_cast<ast::FunctionDecl>(&node)) name(node, n->name, is_strict(n) || (n->body && prologue_strict(n->body->body)), n->isAsync, n->isGenerator);
    if (const auto* n = cursiv::ast_cast<ast::FunctionExpr>(&node)) name(node, n->name, is_strict(n) || (n->body && prologue_strict(n->body->body)), n->isAsync, n->isGenerator);
    if (const auto* n = cursiv::ast_cast<ast::ClassDecl>(&node)) name(node, n->name, true);
    if (const auto* n = cursiv::ast_cast<ast::ClassExpr>(&node)) name(node, n->name, true);
    if (const auto* n = cursiv::ast_cast<ast::ImportSpecifier>(&node)) {
      auto binding = n->local.empty() ? n->imported : n->local;
      if (!binding.empty() && (binding[0] == '\'' || binding[0] == '"'))
        accept.error(*n, "A string-named import requires a local binding.", "javascript.import-binding");
      else name(node, binding, true, true);
    }
    if (const auto* n = cursiv::ast_cast<ast::LabeledStmt>(&node)) name(node, n->label, is_strict(n));
    if (const auto* n = cursiv::ast_cast<ast::BreakStmt>(&node)) name(node, n->label, is_strict(n));
    if (const auto* n = cursiv::ast_cast<ast::ContinueStmt>(&node)) name(node, n->label, is_strict(n));
    services.astReflection->visitMembers(node,
        [&](std::string_view, std::uint32_t, cursiv::AstNode& child) { self(self, child); });
  };
  walk(walk, program);
}
void JavaScriptValidator::checkString(const ast::StringLit& n, cursiv::ValidationAcceptor& a) const {
  if (n.text.size() < 2) return;
  const auto state = escapes(n.text.substr(1, n.text.size() - 2));
  if (state.invalid) a.error(n, "Invalid string escape.", "javascript.escape");
  else if (state.legacy && is_strict(&n)) a.error(n, "Legacy string escapes are not allowed in strict mode.", "javascript.strict-escape");
}
void JavaScriptValidator::checkTemplate(const ast::TemplateLit& n, cursiv::ValidationAcceptor& a) const {
  if (n.tag) return;
  for (const auto quasi : n.quasis) {
    const auto state = escapes(quasi);
    if (state.invalid || state.legacy) { a.error(n, "Invalid escape in an untagged template.", "javascript.template-escape"); break; }
  }
}
void JavaScriptValidator::checkRegex(const ast::RegexLit& n, cursiv::ValidationAcceptor& a) const {
  const auto slash = n.text.rfind('/'); if (slash == n.text.npos) return;
  std::string flags;
  for (const char c : n.text.substr(slash + 1)) {
    if (std::string_view("dgimsuvy").find(c) == std::string_view::npos || flags.find(c) != flags.npos ||
        (c == 'u' && flags.find('v') != flags.npos) || (c == 'v' && flags.find('u') != flags.npos)) {
      a.error(n, "Invalid or duplicate regular-expression flags.", "javascript.regex-flags"); return;
    }
    flags += c;
  }
}
void JavaScriptValidator::checkFunction(const ast::FunctionDecl& n, cursiv::ValidationAcceptor& a) const { parameters(n.params, n.body, a); }
void JavaScriptValidator::checkFunctionExpr(const ast::FunctionExpr& n, cursiv::ValidationAcceptor& a) const { parameters(n.params, n.body, a); }
void JavaScriptValidator::checkArrow(const ast::ArrowFunctionExpr& n, cursiv::ValidationAcceptor& a) const { parameters(n.params, n.block, a); }
void JavaScriptValidator::checkProperty(const ast::Prop& n, cursiv::ValidationAcceptor& a) const { accessor(n.isGetter, n.isSetter, n.value, n, a); }
void JavaScriptValidator::checkMember(const ast::ClassMember& n, cursiv::ValidationAcceptor& a) const {
  accessor(n.isGetter, n.isSetter, n.value, n, a);
  if (n.computed) return;
  const auto name = property_name(n.key);
  const bool field = !n.isMethod && !n.isGetter && !n.isSetter && !n.staticBlock;
  const auto* id = cursiv::ast_cast<ast::Ident>(n.key);
  if ((id && id->name == "#constructor") ||
      (name == "constructor" && (field || (!n.isStatic && (n.isAsync || n.isGenerator || n.isGetter || n.isSetter)))))
    a.error(n, "Invalid constructor class element.", "javascript.constructor");
  if (n.isStatic && name == "prototype") a.error(n, "A static class element cannot be named prototype.", "javascript.static-prototype");
}
void JavaScriptValidator::checkWith(const ast::WithStmt& n, cursiv::ValidationAcceptor& a) const {
  if (is_strict(&n)) a.error(n, "with is not allowed in strict mode.", "javascript.strict-with");
}
void JavaScriptValidator::checkNewTarget(const ast::NewTargetExpr& n, cursiv::ValidationAcceptor& a) const {
  for (const auto* p = n.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::FunctionDecl>(p) || cursiv::ast_cast<ast::FunctionExpr>(p) || cursiv::ast_cast<ast::ClassMember>(p)) return;
  a.error(n, "new.target requires a function or class-element context.", "javascript.new-target");
}

void register_validation_checks(JavaScriptCoreServices& services) {
  using V = JavaScriptValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNames>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLegacyOctal>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTemplate>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRegex>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunctionExpr>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArrow>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProperty>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMember>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkWith>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNewTarget>(validator)});
}

}  // namespace javascript
