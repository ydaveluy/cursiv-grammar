#include "squirrel/core/validation/SquirrelValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "squirrel/core/CoreServices.hpp"

namespace squirrel {
namespace {
void checkParams(const auto& function, cursiv::ValidationAcceptor& accept) {
  bool sawDefault = false;
  for (const auto* param : function.params) {
    if (param->defaultValue) sawDefault = true;
    else if (sawDefault)
      accept.error(*param, "A parameter after a defaulted parameter needs a default.",
                   "squirrel.parameter-default");
  }
  if (sawDefault && function.variadic)
    accept.error(function, "Default parameters cannot be combined with varargs.",
                 "squirrel.parameter-varargs");
}
}
void SquirrelValidator::checkFunction(const ast::FunctionDecl& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkFunctionExpr(const ast::FunctionExpr& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkLambda(const ast::LambdaExpr& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }
void SquirrelValidator::checkConstructor(const ast::ConstructorDecl& n, cursiv::ValidationAcceptor& a) const { checkParams(n, a); }

void SquirrelValidator::checkRawCall(const ast::RawCallExpr& call,
                                    cursiv::ValidationAcceptor& accept) const {
  if (call.args.size() < 2)
    accept.error(call, "rawcall requires a callee and a receiver.",
                 "squirrel.rawcall-arity");
}

void SquirrelValidator::checkClassTarget(const ast::ClassDecl& declaration,
                                        cursiv::ValidationAcceptor& accept) const {
  const auto* target = declaration.target;
  if (!target) return;
  // These Factor forms are EXPR or LOCAL in the native compiler. Resolving
  // whether an ordinary name denotes a local/outer/constant is a later task.
  const bool expression =
      cursiv::ast_cast<ast::ThisExpr>(target) ||
      cursiv::ast_cast<ast::SourceConstant>(target) ||
      cursiv::ast_cast<ast::ParenExpr>(target) ||
      cursiv::ast_cast<ast::CallExpr>(target) ||
      cursiv::ast_cast<ast::RawCallExpr>(target) ||
      cursiv::ast_cast<ast::IntLit>(target) ||
      cursiv::ast_cast<ast::FloatLit>(target) ||
      cursiv::ast_cast<ast::StringLit>(target) ||
      cursiv::ast_cast<ast::BoolLit>(target) ||
      cursiv::ast_cast<ast::NullLit>(target) ||
      cursiv::ast_cast<ast::ArrayExpr>(target) ||
      cursiv::ast_cast<ast::TableExpr>(target) ||
      cursiv::ast_cast<ast::FunctionExpr>(target) ||
      cursiv::ast_cast<ast::LambdaExpr>(target) ||
      cursiv::ast_cast<ast::ClassExpr>(target);
  const auto* member = cursiv::ast_cast<ast::MemberExpr>(target);
  const auto* index = cursiv::ast_cast<ast::IndexExpr>(target);
  const bool base_access =
      (member && cursiv::ast_cast<ast::BaseExpr>(member->object)) ||
      (index && cursiv::ast_cast<ast::BaseExpr>(index->object));
  if (expression || base_access)
    accept.error(declaration, "This expression cannot be a class declaration target.",
                 "squirrel.class-target").property<&ast::ClassDecl::target>();
}

namespace {
void memberName(const ast::FunctionDecl& function,
                cursiv::ValidationAcceptor& accept) {
  if (function.path.size() > 1)
    accept.error(function, "A table or class method needs one name.",
                 "squirrel.member-name").property<&ast::FunctionDecl::path>();
}
}

void SquirrelValidator::checkMemberName(const ast::ClassMember& member,
                                       cursiv::ValidationAcceptor& accept) const {
  if (const auto* function = std::get_if<ast::FunctionDecl*>(&member.member);
      function && *function)
    memberName(**function, accept);
}

void SquirrelValidator::checkTableNames(const ast::TableExpr& table,
                                       cursiv::ValidationAcceptor& accept) const {
  for (const auto& item : table.items)
    if (const auto* function = std::get_if<ast::FunctionDecl*>(&item);
        function && *function)
      memberName(**function, accept);
}

void SquirrelValidator::checkLocalFunctionName(const ast::LocalFunctionDecl& local,
                                              cursiv::ValidationAcceptor& accept) const {
  if (local.func && local.func->path.size() > 1)
    accept.error(*local.func, "A local function needs one name.",
                 "squirrel.local-function-name").property<&ast::FunctionDecl::path>();
}

void register_validation_checks(SquirrelCoreServices& services) {
  services.validator = std::make_unique<const SquirrelValidator>(services);
  using V = SquirrelValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunctionExpr>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLambda>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstructor>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkRawCall>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkClassTarget>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkMemberName>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkTableNames>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLocalFunctionName>(validator)});
}

}  // namespace squirrel
