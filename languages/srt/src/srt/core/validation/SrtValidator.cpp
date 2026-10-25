#include "srt/core/validation/SrtValidator.hpp"

#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "srt/core/CoreServices.hpp"

namespace srt {

void SrtValidator::checkSubtitle(const ast::Subtitle& n,
                                cursiv::ValidationAcceptor& a) const {
  for (auto time : {n.start, n.end}) {
    while (!time.empty()) {
      const auto begin = time.find_first_of("+-0123456789");
      if (begin == std::string_view::npos) break;
      time.remove_prefix(begin);
      const bool negative = time.front() == '-';
      const auto sign = time.front() == '+' || negative ? 1u : 0u;
      auto end = time.find_first_not_of("0123456789", sign);
      if (end == std::string_view::npos) end = time.size();
      const auto component = time.substr(0, end);
      auto digits = component.substr(sign);
      const auto first = digits.find_first_not_of('0');
      digits = first == std::string_view::npos ? std::string_view("0") : digits.substr(first);
      const std::string_view limit = negative ? "2147483648" : "2147483647";
      if (digits.size() > limit.size() || (digits.size() == limit.size() && digits > limit))
        a.error(n, "A timestamp component must fit a signed 32-bit integer.").range(a.spanOfName(component, n));
      if (!end) break;
      time.remove_prefix(end);
    }
  }
}

void register_validation_checks(SrtCoreServices& services) {
  using V = SrtValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &V::checkSubtitle>(validator)});
}

}  // namespace srt
