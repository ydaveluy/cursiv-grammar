#include "url/core/validation/UrlValidator.hpp"

#include <charconv>
#include <cstdint>
#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "url/core/CoreServices.hpp"

namespace url {
namespace {
bool hex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}
// RFC 3986 unreserved / sub-delims / pct-encoded, with component extras.
bool encoded(std::string_view text, std::string_view extras = {}, bool subdelims = true) {
  for (std::size_t i = 0; i < text.size(); ++i) {
    const char c = text[i];
    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
        std::string_view("-._~").find(c) != std::string_view::npos ||
        (subdelims && std::string_view("!$&'()*+,;=").find(c) != std::string_view::npos) ||
        extras.find(c) != std::string_view::npos) continue;
    if (c != '%' || i + 2 >= text.size() || !hex(text[i + 1]) || !hex(text[i + 2])) return false;
    i += 2;
  }
  return true;
}

bool ipLiteral(std::string_view text) {
  if (text.starts_with('v') || text.starts_with('V')) {
    const auto dot = text.find('.');
    if (dot == std::string_view::npos || dot < 2 || dot + 1 == text.size()) return false;
    for (char c : text.substr(1, dot - 1)) if (!hex(c)) return false;
    const auto address = text.substr(dot + 1);
    return address.find('%') == std::string_view::npos && encoded(address, ":");
  }
  if (const auto zone = text.find("%25"); zone != std::string_view::npos) {
    const auto id = text.substr(zone + 3);
    if (id.empty() || !encoded(id, {}, false)) return false;
    text = text.substr(0, zone);
  }
  auto groups = [](std::string_view part, bool allowIpv4) {
    int count = 0;
    while (!part.empty()) {
      const auto colon = part.find(':');
      const auto group = part.substr(0, colon);
      if (group.empty()) return -100;
      if (group.find('.') != std::string_view::npos) {
        if (!allowIpv4 || colon != std::string_view::npos) return -100;
        auto rest = group;
        for (int i = 0; i < 4; ++i) {
          const auto dot = rest.find('.');
          const auto number = rest.substr(0, dot);
          unsigned value = 0;
          const auto parsed = std::from_chars(number.data(), number.data() + number.size(), value);
          if (number.empty() || (number.size() > 1 && number[0] == '0') ||
              parsed.ec != std::errc{} || parsed.ptr != number.data() + number.size() ||
              value > 255 || (i == 3) != (dot == std::string_view::npos)) return -100;
          if (i != 3) rest.remove_prefix(dot + 1);
        }
        count += 2;
      } else {
        if (group.size() > 4) return -100;
        for (char c : group) if (!hex(c)) return -100;
        ++count;
      }
      if (colon == std::string_view::npos) break;
      part.remove_prefix(colon + 1);
      if (part.empty()) return -100;
    }
    return count;
  };
  const auto compressed = text.find("::");
  if (compressed == std::string_view::npos) return groups(text, true) == 8;
  if (text.find("::", compressed + 2) != std::string_view::npos) return false;
  const int left = groups(text.substr(0, compressed), false);
  const int right = groups(text.substr(compressed + 2), true);
  return left >= 0 && right >= 0 && left + right < 8;
}
}  // namespace

void UrlValidator::checkPortInRange(const ast::Url& url,
                                    cursiv::ValidationAcceptor& accept) const {
  const auto port = url.port();
  if (!port || port->empty()) return;
  std::uint64_t number = 0;
  const auto text = *port;
  const auto conversion = std::from_chars(text.data(), text.data() + text.size(), number);
  if (conversion.ec == std::errc::invalid_argument ||
      conversion.ptr != text.data() + text.size())
    accept.error(*url.host, "Port must contain decimal digits.", "url.port-digits")
        .property<&ast::Host::text>();
  else if (conversion.ec == std::errc::result_out_of_range || number > 65535)
    accept.warning(*url.host, "Port is outside the 0-65535 range.", "url.port-range")
        .property<&ast::Host::text>();
}

void UrlValidator::checkComponents(
    const ast::Url& url, cursiv::ValidationAcceptor& accept) const {
  if (url.login && !encoded(url.login->text, ":"))
    accept.error(*url.login, "Userinfo needs valid URI characters or percent escapes.", "url.userinfo")
        .property<&ast::Login::text>();
  if (url.host) {
    const auto text = url.host->text;
    const auto close = text.find(']');
    const bool valid = text.starts_with('[')
        ? close != std::string_view::npos && close > 1 &&
          text.find(']', close + 1) == std::string_view::npos &&
          (close + 1 == text.size() ||
           (text[close + 1] == ':' && text.find(':', close + 2) == std::string_view::npos)) &&
          ipLiteral(text.substr(1, close - 1))
        : encoded(url.host->name());
    if (!valid)
      accept.error(*url.host, "Malformed URI host.", "url.host")
          .property<&ast::Host::text>();
  }
  if (url.path)
    for (const auto segment : url.path->segments)
      if (!encoded(segment, ":@")) {
        accept.error(*url.path, "Path segments need valid URI characters or percent escapes.",
                     "url.path").property<&ast::Path::segments>();
        break;
      }
  for (const auto* parameter : url.query) {
    if (!encoded(parameter->key, ":@/?"))
      accept.error(*parameter, "Query keys need valid URI characters or percent escapes.",
                   "url.query").property<&ast::SearchParameter::key>();
    if (!encoded(parameter->value, ":@/?"))
      accept.error(*parameter, "Query values need valid URI characters or percent escapes.",
                   "url.query").property<&ast::SearchParameter::value>();
  }
  if (!encoded(url.frag, ":@/?"))
    accept.error(url, "A fragment needs valid URI characters or percent escapes.", "url.fragment")
        .property<&ast::Url::frag>();
}

void register_validation_checks(UrlCoreServices& services) {
  using V = UrlValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkPortInRange>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkComponents>(validator)});
}

}  // namespace url
