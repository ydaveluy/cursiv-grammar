#include "romannumerals/core/validation/RomannumeralsValidator.hpp"

#include <memory>
#include <string>
#include <utility>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "romannumerals/core/CoreServices.hpp"

namespace romannumerals {

void RomannumeralsValidator::checkCanonical(
    const ast::RomanNumeral& node, cursiv::ValidationAcceptor& accept) const {
  std::string text(node.text);
  for (char& c : text)
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
  if (text == "N") return;
  const auto digit = [](char c) {
    switch (c) {
      case 'M':
        return 1000;
      case 'D':
        return 500;
      case 'C':
        return 100;
      case 'L':
        return 50;
      case 'X':
        return 10;
      case 'V':
        return 5;
      default:
        return 1;
    }
  };
  std::int64_t n = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const int d = digit(text[i]);
    n += i + 1 < text.size() && d < digit(text[i + 1]) ? -d : d;
  }
  std::string canonical;
  for (const auto& [v, t] : {std::pair{1000, "M"},
                             {900, "CM"},
                             {500, "D"},
                             {400, "CD"},
                             {100, "C"},
                             {90, "XC"},
                             {50, "L"},
                             {40, "XL"},
                             {10, "X"},
                             {9, "IX"},
                             {5, "V"},
                             {4, "IV"},
                             {1, "I"}})
    while (n >= v) {
      canonical += t;
      n -= v;
    }
  if (canonical != text)
    accept.error(node, "Non-canonical Roman numeral.")
        .property<&ast::RomanNumeral::text>()
        .code("romannumerals.canonical");
}

void register_validation_checks(RomannumeralsCoreServices& services) {
  using V = RomannumeralsValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkCanonical>(
          validator)});
}

}  // namespace romannumerals
