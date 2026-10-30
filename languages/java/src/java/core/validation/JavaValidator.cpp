#include "java/core/validation/JavaValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "java/core/CoreServices.hpp"

namespace java {
namespace {
template<class Params>
void check_params(const Params& params, cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 0; i < params.size(); ++i) {
    const auto* p = params[i];
    if (!p || !p->varargs) continue;
    if (i + 1 != params.size())
      accept.error(*p, "Only the last parameter may be varargs.", "java.varargs-last");
    if (!p->extraDims.empty())
      accept.error(*p, "A varargs parameter cannot have trailing array brackets.", "java.varargs-dims");
  }
}
} // namespace

void JavaValidator::checkMethod(const ast::MethodDecl& n,
                                cursiv::ValidationAcceptor& a) const { check_params(n.params, a); }
void JavaValidator::checkConstructor(const ast::ConstructorDecl& n,
                                     cursiv::ValidationAcceptor& a) const { check_params(n.params, a); }
void JavaValidator::checkLambda(const ast::LambdaExpr& n,
                                cursiv::ValidationAcceptor& a) const {
  check_params(n.params, a);
  bool var = false, typed = false;
  for (const auto* p : n.params) if (p) { var |= p->isVar; typed |= p->type != nullptr; }
  if (var && typed)
    a.error(n, "A lambda cannot mix var with explicit parameter types.", "java.lambda-parameter-kind");
}
void JavaValidator::checkCall(const ast::CallExpr& n,
                              cursiv::ValidationAcceptor& a) const {
  const auto* ident = cursiv::ast_cast<ast::Ident>(n.callee);
  if (ident && ident->name == "yield")
    a.error(n, "A method named yield must be invoked with a qualifier.", "java.yield-call");
}
void JavaValidator::checkGenericCall(const ast::ExplicitGenericInvokeExpr& n,
                                     cursiv::ValidationAcceptor& a) const {
  if (n.target == "yield")
    a.error(n, "A method named yield must be invoked with a qualifier.", "java.yield-call");
}
void JavaValidator::checkPattern(const ast::TypePattern& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (n.isVar && !cursiv::ast_cast<ast::RecordPattern>(n.parent))
    a.error(n, "An inferred var pattern is a record-pattern component.", "java.pattern-var");
  if (cursiv::ast_cast<ast::PrimitiveTypeRef>(n.type) &&
      !cursiv::ast_cast<ast::RecordPattern>(n.parent))
    a.error(n, "A top-level type pattern must use a reference type.", "java.pattern-reference");
}

// The grammar accepts `...` on any component; only the last may vary.
void JavaValidator::checkRecordComponentVarargsIsLast(
    const ast::RecordDecl& record, cursiv::ValidationAcceptor& accept) const {
  const auto count = record.components.size();
  for (std::size_t i = 0; i + 1 < count; ++i) {
    const ast::RecordComponent* const component = record.components[i];
    if (component != nullptr && component->varargs)
      accept.error(*component, "Only the last record component may be varargs.");
  }
}

void register_validation_checks(JavaCoreServices& services) {
  using V = JavaValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkRecordComponentVarargsIsLast>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMethod>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstructor>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLambda>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGenericCall>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPattern>(validator)});
}

}  // namespace java
