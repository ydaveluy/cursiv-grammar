#include "json5/core/validation/Json5Validator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "json5/core/CoreServices.hpp"

namespace json5 {

void Json5Validator::checkNumber(const ast::Number &number,
                                 cursiv::ValidationAcceptor &accept) const {
  auto text = number.text;
  if (!text.empty() && (text[0] == '+' || text[0] == '-'))
    text.remove_prefix(1);
  if (text.starts_with("0x") || text.starts_with("0X"))
    return;
  if (const auto e = text.find_first_of("eE"); e != std::string_view::npos) {
    auto exponent = text.substr(e + 1);
    if (!exponent.empty() && (exponent[0] == '+' || exponent[0] == '-'))
      exponent.remove_prefix(1);
    if (exponent.empty())
      accept.error(number, "The exponent needs at least one digit.")
          .property<&ast::Number::text>();
  }
}

void register_validation_checks(Json5CoreServices &services) {
  services.validator = std::make_unique<const Json5Validator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &Json5Validator::checkNumber>(*services.validator)});
}

} // namespace json5
