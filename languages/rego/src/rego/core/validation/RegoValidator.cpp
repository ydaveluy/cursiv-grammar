#include "rego/core/validation/RegoValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "rego/core/CoreServices.hpp"

namespace rego {
namespace {
bool reference(const ast::Expr* n) {
  if (cursiv::ast_cast<ast::Ref>(n)) return true;
  const auto* a = cursiv::ast_cast<ast::AccessExpr>(n);
  return a && reference(a->target);
}
bool variable(const ast::Expr* n) {
  const auto* r = cursiv::ast_cast<ast::Ref>(n);
  return r && r->parts.empty();
}
}  // namespace
void RegoValidator::checkNumber(const ast::NumberLiteral& n,
                                cursiv::ValidationAcceptor& a) const {
  if (n.text.size() > 1 && n.text[0] == '0' && n.text[1] >= '0' &&
      n.text[1] <= '9')
    a.error(n, "a number cannot have a leading zero")
        .property<&ast::NumberLiteral::text>();
}
void RegoValidator::checkScalar(const ast::ScalarTerm& n,
                                cursiv::ValidationAcceptor& a) const {
  if (!n.sign.empty() &&
      (n.sign != "-" || !std::holds_alternative<ast::NumberLiteral*>(n.value)))
    a.error(n, "only numbers may have a leading minus")
        .property<&ast::ScalarTerm::sign>();
}
void RegoValidator::checkQuery(const ast::Query& n,
                               cursiv::ValidationAcceptor& a) const {
  if (n.literals.empty()) a.error(n, "a query body must contain an expression");
}
void RegoValidator::checkLiteral(const ast::LiteralExpr& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (n.rest.size() > 1)
    a.error(n, "an expression may contain only one assignment or unification");
}
void RegoValidator::checkSome(const ast::SomeDecl& n,
                              cursiv::ValidationAcceptor& a) const {
  if (n.negated) a.error(n, "a some declaration cannot be negated");
  if (n.collection) {
    if (n.symbols.size() > 2)
      a.error(n, "membership takes a value or a key and value");
  } else {
    for (const auto* s : n.symbols)
      if (!variable(s)) a.error(*s, "a some declaration requires variables");
  }
}
void RegoValidator::checkEvery(const ast::Every& n,
                               cursiv::ValidationAcceptor& a) const {
  if (n.negated) a.error(n, "an every expression cannot be negated");
  if (n.symbols.size() > 2)
    a.error(n, "every takes a value or a key and value");
  for (const auto* s : n.symbols)
    if (!variable(s)) a.error(*s, "every bindings must be variables");
}
void RegoValidator::checkRule(const ast::Rule& n,
                              cursiv::ValidationAcceptor& a) const {
  if (n.contains && !n.params.empty())
    a.error(n, "a function cannot use contains");
  if (cursiv::ast_cast<ast::DefaultRule>(&n)) {
    if (!n.value) a.error(n, "a default rule requires a value");
    for (const auto* p : n.params)
      if (!variable(p))
        a.error(*p, "default function arguments must be variables");
  }
  bool seenElse = false;
  for (std::size_t i = 0; i < n.bodies.size(); ++i) {
    const auto* b = n.bodies[i];
    if (b->isElse) {
      if (i == 0) a.error(*b, "else requires a preceding rule body");
      if (!b->elseValue && !b->body)
        a.error(*b, "else requires a value or body");
      seenElse = true;
    } else if (seenElse)
      a.error(*b, "a body after else must begin with else");
  }
}
void RegoValidator::checkWith(const ast::WithMod& n,
                              cursiv::ValidationAcceptor& a) const {
  if (!reference(n.target))
    a.error(n, "a with target must be a reference")
        .property<&ast::WithMod::target>();
}
void register_validation_checks(RegoCoreServices& services) {
  using V = RegoValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkScalar>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkQuery>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLiteral>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSome>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkEvery>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRule>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkWith>(v)});
}
}  // namespace rego
