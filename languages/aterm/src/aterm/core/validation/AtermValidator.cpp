#include "aterm/core/validation/AtermValidator.hpp"
#include "aterm/core/CoreServices.hpp"
#include "aterm/core/Value.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include <charconv>
#include <cstdint>
#include <memory>
namespace aterm {
void AtermValidator::checkNumber(const ast::Number &n,
                                 cursiv::ValidationAcceptor &a) const {
  const auto s = n.raw;
  std::size_t i = s.starts_with('-') ? 1 : 0;
  bool digits = false;
  while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
    digits = true;
    ++i;
  }
  if (i < s.size() && s[i] == '.') {
    ++i;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') {
      digits = true;
      ++i;
    }
  }
  bool valid = digits;
  if (i < s.size() && (s[i] == 'e' || s[i] == 'E')) {
    ++i;
    if (i < s.size() && (s[i] == '+' || s[i] == '-'))
      ++i;
    auto begin = i;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9')
      ++i;
    valid &= i > begin;
  }
  if (!valid || i != s.size()) {
    a.error(n, "Expected a decimal ATerm number with digits in its exponent.")
        .code("aterm.number");
    return;
  }
  if (real(n)) {
    double v;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec == std::errc::result_out_of_range)
      a.error(n, "The real number is outside the finite double range; its "
                 "source text is retained.")
          .code("aterm.real-range");
  } else {
    std::int32_t v;
    const auto r = std::from_chars(s.data(), s.data() + s.size(), v);
    if (r.ec == std::errc::result_out_of_range)
      a.error(n, "The integer is outside the signed 32-bit ATerm range; its "
                 "source text is retained.")
          .code("aterm.integer-range");
  }
}
void register_validation_checks(AtermCoreServices &services) {
  using V = AtermValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(
          *services.validator)});
}
} // namespace aterm
