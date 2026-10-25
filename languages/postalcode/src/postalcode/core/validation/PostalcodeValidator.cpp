#include "postalcode/core/validation/PostalcodeValidator.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "postalcode/core/CoreServices.hpp"

namespace postalcode {

namespace {
constexpr std::string_view kExcludedLetters = "DFIOQU";
char upper(char c) { return c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c; }
}  // namespace

void PostalcodeValidator::checkNoExcludedLetters(
    const ast::PostalCode& code, cursiv::ValidationAcceptor& accept) const {
  if (code.forwardSortationArea.size() != 3 ||
      code.localDeliveryUnit.size() != 3) {
    accept.warning(code,
                   "Each postal code group must contain three characters.");
    return;
  }
  for (const char letter :
       {code.forwardSortationArea[0], code.forwardSortationArea[2],
        code.localDeliveryUnit[1]})
    if (kExcludedLetters.find(upper(letter)) != std::string_view::npos)
      accept.warning(code, "Canadian postal codes do not use the letter '" +
                               std::string(1, letter) + "'.");
  if (upper(code.forwardSortationArea[0]) == 'W' ||
      upper(code.forwardSortationArea[0]) == 'Z')
    accept.warning(code, "Canadian postal codes cannot begin with W or Z.");
}

void register_validation_checks(PostalcodeCoreServices& services) {
  using V = PostalcodeValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkNoExcludedLetters>(validator)});
}

}  // namespace postalcode
