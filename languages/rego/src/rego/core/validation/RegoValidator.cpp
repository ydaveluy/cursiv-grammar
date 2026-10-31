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
bool reserved(std::string_view s) {
  return s=="package"||s=="import"||s=="as"||s=="default"||s=="else"||
      s=="not"||s=="some"||s=="with"||s=="true"||s=="false"||s=="null";
}
bool stringPart(const ast::RefPart& p) {
  if (std::holds_alternative<std::string_view>(p)) return true;
  const auto* x=cursiv::ast_cast<ast::ScalarTerm>(std::get<ast::Expr*>(p));
  return x && x->sign.empty() && std::holds_alternative<std::string_view>(x->value);
}
bool staticPath(const ast::Ref* r) {
  if (!r) return false;
  for (const auto& p:r->parts) if(!stringPart(p)) return false;
  return true;
}
// Comprehensions introduce their own body and are valid default values;
// variables/calls in ordinary containers are forbidden by OPA's parser.
bool defaultValue(const ast::Expr* e) {
  if(!e) return false;
  if(cursiv::ast_cast<ast::ScalarTerm>(e) || cursiv::ast_cast<ast::ArrayCompr>(e) ||
     cursiv::ast_cast<ast::SetCompr>(e) || cursiv::ast_cast<ast::ObjectCompr>(e)) return true;
  if(const auto* a=cursiv::ast_cast<ast::ArrayLit>(e)) {
    for(const auto* x:a->items) if(!defaultValue(x)) return false;
    return true;
  }
  if(const auto* a=cursiv::ast_cast<ast::SetLit>(e)) {
    for(const auto* x:a->items) if(!defaultValue(x)) return false;
    return true;
  }
  if(const auto* a=cursiv::ast_cast<ast::ObjectLit>(e)) {
    for(const auto* x:a->items) if(!defaultValue(x->key)||!defaultValue(x->value)) return false;
    return true;
  }
  return false;
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
  if (n.contains && n.hasParams)
    a.error(n, "a function cannot use contains", "rego.function-contains");
  const bool bareVariable = !n.hasParams && n.parts.empty();
  const bool emptyCall = n.hasParams && n.params.empty();
  if (!cursiv::ast_cast<ast::DefaultRule>(&n) && (bareVariable || emptyCall) &&
      !n.value && !n.contains && n.bodies.empty()) {
    if (const auto* document = cursiv::ast_cast<ast::Document>(n.parent))
      for (const auto* statement : document->statements)
        if (cursiv::ast_cast<ast::PackageDecl>(statement)) {
          if (bareVariable)
            a.error(n, "a bare variable cannot define a module rule",
                    "rego.module-rule-name");
          else
            a.error(n, "a bodyless function rule needs an argument or value",
                    "rego.module-rule-arguments");
          break;
        }
  }
  if (cursiv::ast_cast<ast::DefaultRule>(&n)) {
    if (!n.value) a.error(n, "a default rule requires a value");
    else if (!defaultValue(n.value)) a.error(n, "a default value cannot contain variables or calls");
    for (std::size_t i=0;i<n.params.size();++i) {
      const auto* p=n.params[i];
      if (!variable(p)) a.error(*p,"default function arguments must be variables");
      else {
        const auto* r=cursiv::ast_cast<ast::Ref>(p);
        if(r->name!="_") for(std::size_t j=0;j<i;++j) {
          const auto* previous=cursiv::ast_cast<ast::Ref>(n.params[j]);
          if(previous&&previous->name==r->name) a.error(*p,"default function arguments cannot repeat");
        }
      }
    }
  }
  bool seenElse = false;
  for (std::size_t i = 0; i < n.bodies.size(); ++i) {
    const auto* b = n.bodies[i];
    if (b->isElse) {
      if(n.contains) a.error(*b, "a multi-value rule cannot use else");
      if (i == 0) a.error(*b, "else requires a preceding rule body");
      if (!b->elseValue && !b->body)
        a.error(*b, "else requires a value or body");
      seenElse = true;
    } else if (seenElse)
      a.error(*b, "a body after else must begin with else");
  }
}
void RegoValidator::checkPackage(const ast::PackageDecl& n, cursiv::ValidationAcceptor& a) const {
  if(!staticPath(n.path)) a.error(n,"a package path must contain only names or string keys");
}
void RegoValidator::checkImport(const ast::ImportDecl& n, cursiv::ValidationAcceptor& a) const {
  if(!staticPath(n.path)) a.error(n,"an import path must contain only names or string keys");
  if(n.alias && (!n.alias->parts.empty()||reserved(n.alias->name)))
    a.error(n,"an import alias must be a non-keyword variable");
  if(!n.alias&&n.path) {
    std::string_view tail=n.path->name;
    if(!n.path->parts.empty()) {
      const auto& part=n.path->parts[n.path->parts.size()-1];
      if(const auto* text=std::get_if<std::string_view>(&part)) tail=*text;
      else if(const auto* value=cursiv::ast_cast<ast::ScalarTerm>(std::get<ast::Expr*>(part)))
        if(const auto* text=std::get_if<std::string_view>(&value->value)) tail=*text;
    }
    if(reserved(tail)) a.error(n,"an import path ending in a keyword requires an alias");
  }
}
void RegoValidator::checkRef(const ast::Ref& n, cursiv::ValidationAcceptor& a) const {
  if(!reserved(n.name)) return;
  const ast::Expr* root=&n;
  for(;;) {
    if(const auto* p=cursiv::ast_cast<ast::AccessExpr>(root->parent);p&&p->target==root) root=p;
    else if(const auto* p=cursiv::ast_cast<ast::Call>(root->parent);p&&p->target==root) root=p;
    else if(const auto* p=cursiv::ast_cast<ast::BinaryExpr>(root->parent);p&&p->left==root) root=p;
    else break;
  }
  const auto* literal=cursiv::ast_cast<ast::LiteralExpr>(root->parent);
  if(!literal||literal->first!=root) a.error(n,"a keyword reference is only allowed at the start of a query literal");
}
void RegoValidator::checkAccess(const ast::AccessExpr& n, cursiv::ValidationAcceptor& a) const {
  if(cursiv::ast_cast<ast::ScalarTerm>(n.target)) a.error(n,"a scalar cannot head a reference");
}
void RegoValidator::checkCall(const ast::Call& n, cursiv::ValidationAcceptor& a) const {
  if(cursiv::ast_cast<ast::ScalarTerm>(n.target)) a.error(n,"a scalar cannot head a call");
  if(cursiv::ast_cast<ast::Call>(n.target)) a.error(n,"a call cannot head another call");
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
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPackage>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkImport>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRef>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAccess>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkWith>(v)});
}
}  // namespace rego
