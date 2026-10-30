#include "istc/core/validation/IstcValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "istc/core/CoreServices.hpp"

namespace istc {

void IstcValidator::checkCode(const ast::Istc& code,
                              cursiv::ValidationAcceptor& accept) const {
  if (code.year.find_first_not_of("0123456789") != std::string_view::npos)
    accept.error(code, "The registration year must contain decimal digits.",
                 "istc.year").property<&ast::Istc::year>();

  // ISO 21047:2009, Annex A: MOD 16-3 weights, from left to right.
  constexpr unsigned weights[] = {11, 9, 3, 1};
  unsigned sum = 0;
  std::size_t position = 0;
  for (const auto part : {code.registration, code.year, code.work}) {
    for (const char c : part) {
      const auto digit = std::string_view("0123456789ABCDEF").find(c);
      if (digit == std::string_view::npos)
        return;
      sum += static_cast<unsigned>(digit) * weights[position++ % 4];
    }
  }
  if (position != 15 || code.check.size() != 1)
    return;
  if (code.check.front() != "0123456789ABCDEF"[sum % 16])
    accept.error(code, "The ISTC check digit does not match its components.",
                 "istc.check").property<&ast::Istc::check>();
}

void register_validation_checks(IstcCoreServices& services) {
  services.validator = std::make_unique<const IstcValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &IstcValidator::checkCode>(*services.validator)});
}

}  // namespace istc
