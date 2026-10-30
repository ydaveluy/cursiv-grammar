#include "muparser/core/validation/MuparserValidator.hpp"

#include <array>
#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "muparser/core/CoreServices.hpp"

namespace muparser {

void MuparserValidator::checkArity(const ast::FunctionCall& call,
                                   cursiv::ValidationAcceptor& accept) const {
  static constexpr std::array<std::string_view, 21> unary = {
      "sin",  "cos",   "tan",   "asin",  "acos", "atan",  "sinh", "cosh",
      "tanh", "asinh", "acosh", "atanh", "log2", "log10", "log",  "ln",
      "exp",  "sqrt",  "sign",  "rint",  "abs"};
  bool valid = true;
  if (call.name == "rnd")
    valid = call.args.empty();
  else if (call.name == "atan2")
    valid = call.args.size() == 2;
  else if (call.name == "min" || call.name == "max" || call.name == "sum" ||
           call.name == "avg")
    valid = !call.args.empty();
  else
    for (const auto name : unary)
      if (call.name == name) valid = call.args.size() == 1;
  if (!valid)
    accept.error(call, "Invalid number of arguments for this builtin function.",
                 "muparser.arity");
}

void MuparserValidator::checkTarget(const ast::AssignExpr& assignment,
                                    cursiv::ValidationAcceptor& accept) const {
  if (assignment.target &&
      !cursiv::ast_cast<ast::VariableRef>(assignment.target))
    accept
        .error(assignment, "An assignment target must be a variable.",
               "muparser.target")
        .property<&ast::AssignExpr::target>();
}

void register_validation_checks(MuparserCoreServices& services) {
  using V = MuparserValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkArity>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTarget>(
           *services.validator)});
}

}  // namespace muparser
