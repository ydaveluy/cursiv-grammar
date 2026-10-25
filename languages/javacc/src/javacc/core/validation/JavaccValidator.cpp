#include "javacc/core/validation/JavaccValidator.hpp"

#include <memory>
#include <string>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "javacc/core/CoreServices.hpp"

namespace javacc {

// JavaCC itself refuses when the two names differ: `PARSER_BEGIN(A) ...
// PARSER_END(B)` names two different classes with one parser body.
void JavaccValidator::checkParserNamesMatch(
    const ast::JavaccInput& input, cursiv::ValidationAcceptor& accept) const {
  if (input.parserBeginName != input.parserEndName)
    accept
        .error(input, "PARSER_END(" + std::string(input.parserEndName) +
                          ") does not match PARSER_BEGIN(" +
                          std::string(input.parserBeginName) + ")")
        .property<&ast::JavaccInput::parserEndName>();
}

void register_validation_checks(JavaccCoreServices& services) {
  using V = JavaccValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkParserNamesMatch>(validator)});
}

}  // namespace javacc
