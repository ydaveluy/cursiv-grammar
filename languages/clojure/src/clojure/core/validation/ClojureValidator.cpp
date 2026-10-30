#include "clojure/core/validation/ClojureValidator.hpp"

#include <charconv>
#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "clojure/core/CoreServices.hpp"

namespace clojure {

void ClojureValidator::checkRadix(const ast::RadixLit& value,
                                  cursiv::ValidationAcceptor& accept) const {
  std::string_view text = value.text;
  if (text.starts_with('+') || text.starts_with('-')) text.remove_prefix(1);
  const auto split = text.find_first_of("rR");
  unsigned radix = 0;
  std::from_chars(text.data(), text.data() + split, radix);
  bool valid = radix >= 2 && radix <= 36;
  for (const char c : text.substr(split + 1)) {
    const unsigned digit = c >= '0' && c <= '9' ? c - '0'
                           : c >= 'a' && c <= 'z' ? c - 'a' + 10
                                                  : c - 'A' + 10;
    valid = valid && digit < radix;
  }
  if (!valid)
    accept.error(value, "Expected digits in radix 2 through 36.",
                 "clojure.radix").property<&ast::RadixLit::text>();
}

void register_validation_checks(ClojureCoreServices& services) {
  services.validator = std::make_unique<const ClojureValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &ClojureValidator::checkRadix>(*services.validator)});
}

}  // namespace clojure
