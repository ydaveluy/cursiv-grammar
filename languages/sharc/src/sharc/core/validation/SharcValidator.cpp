#include "sharc/core/validation/SharcValidator.hpp"

#include <charconv>
#include <memory>
#include <string_view>

#include "cursiv/core/utils/CaseFold.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "sharc/core/CoreServices.hpp"

namespace sharc {

namespace {

// Table 1-15: decimal, hexadecimal and binary integer spellings. Precision
// is an integer setting, not an expression or a floating/fractional constant.
bool precision_value(std::string_view text) {
  int base = 10;
  if (text.size() > 2 && cursiv::folded_equal(text.substr(0, 2), "0x")) {
    text.remove_prefix(2);
    base = 16;
  } else if (text.size() > 2 && cursiv::folded_equal(text.substr(0, 2), "b#")) {
    text.remove_prefix(2);
    base = 2;
  }
  if (text.empty()) return false;
  unsigned value = 0;
  const auto [end, error] =
      std::from_chars(text.data(), text.data() + text.size(), value, base);
  return error == std::errc{} && end == text.data() + text.size() &&
         (value == 32 || value == 40);
}

}  // namespace

void SharcValidator::checkDirective(const ast::Directive& directive,
                                    cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::folded_equal(directive.keyword, ".precision") ||
      directive.values.size() != 1) return;
  const auto* number = cursiv::ast_cast<ast::NumberLit>(directive.values[0]);
  if (number && !precision_value(number->value))
    accept.error(*number, "Precision must be the integer 32 or 40.", "sharc.precision")
        .property<&ast::NumberLit::value>();
}

void register_validation_checks(SharcCoreServices& services) {
  services.validator = std::make_unique<const SharcValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&SharcValidator::checkDirective>(
          *services.validator)});
}

}  // namespace sharc
