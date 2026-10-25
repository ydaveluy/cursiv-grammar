#include "amazonstateslanguageintrinsicfunctions/core/validation/AmazonStatesLanguageIntrinsicFunctionsValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "amazonstateslanguageintrinsicfunctions/core/CoreServices.hpp"

namespace amazonstateslanguageintrinsicfunctions {

namespace {

struct Arity {
  int min;
  int max;  // -1: unbounded
};

Arity arity_of(ast::Function function) {
  switch (function) {
    case ast::Function::format:
      return {1, -1};
    case ast::Function::string_to_json:
    case ast::Function::json_to_string:
    case ast::Function::array_length:
    case ast::Function::array_unique:
    case ast::Function::base64_encode:
    case ast::Function::base64_decode:
      return {1, 1};
    case ast::Function::array:
      return {0, -1};
    case ast::Function::array_partition:
    case ast::Function::array_contains:
    case ast::Function::array_get_item:
    case ast::Function::hash:
    case ast::Function::math_add:
    case ast::Function::string_split:
      return {2, 2};
    case ast::Function::array_range:
    case ast::Function::json_merge:
      return {3, 3};
    case ast::Function::math_random:
      return {2, 3};
    case ast::Function::uuid:
      return {0, 0};
  }
  return {0, -1};
}

}  // namespace

void AmazonStatesLanguageIntrinsicFunctionsValidator::checkArgumentCount(
    const ast::FuncCall& call, cursiv::ValidationAcceptor& accept) const {
  const Arity arity = arity_of(call.function);
  const auto count = static_cast<int>(call.args.size());
  if (count < arity.min || (arity.max >= 0 && count > arity.max))
    accept.error(call, "Wrong number of arguments.", "asl-intrinsic.arity");
}

void register_validation_checks(
    AmazonStatesLanguageIntrinsicFunctionsCoreServices& services) {
  using V = AmazonStatesLanguageIntrinsicFunctionsValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkArgumentCount>(validator)});
}

}  // namespace amazonstateslanguageintrinsicfunctions
