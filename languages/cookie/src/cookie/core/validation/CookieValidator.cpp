#include "cookie/core/validation/CookieValidator.hpp"

#include <memory>
#include <string>

#include "cookie/core/Value.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cookie/core/CoreServices.hpp"

namespace cookie {

void CookieValidator::checkDocument(const ast::Document& doc,
                                    cursiv::ValidationAcceptor& accept) const {
  bool cookieSeen = false;
  constexpr std::string_view legal =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!#$%&'*+-.^_`|~:";
  for (const auto* pair : doc.pairs) {
    if (pair->name.find_first_not_of(legal) != pair->name.npos)
      accept.error(*pair, "Invalid cookie name.", "cookie.name")
          .property<&ast::Pair::name>();
    std::string name(pair->name);
    for (auto& c : name)
      if (c >= 'A' && c <= 'Z')
        c += 'a' - 'A';
    const bool flag = name == "secure" || name == "httponly" || name == "partitioned";
    const bool attribute = flag || name == "expires" || name == "path" ||
        name == "comment" || name == "domain" || name == "max-age" ||
        name == "version" || name == "samesite" || name.starts_with('$');
    if (attribute && !name.starts_with('$') && !cookieSeen)
      accept.error(*pair, "An attribute must follow a cookie.", "cookie.attribute");
    if (!pair->value && !flag && !name.starts_with('$'))
      accept.error(*pair, "This cookie or attribute needs a value.", "cookie.value");
    if (!attribute && pair->value)
      cookieSeen = true;
    if (const auto value = decoded_value(*pair))
      for (const unsigned char c : *value)
        if (c < 32 || c == 127) {
          accept.error(*pair, "A cookie value cannot contain control characters.",
                       "cookie.control").property<&ast::Pair::value>();
          break;
        }
  }
}

void register_validation_checks(CookieCoreServices& services) {
  services.validator = std::make_unique<const CookieValidator>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&CookieValidator::checkDocument>(
          *services.validator)});
}

}  // namespace cookie
