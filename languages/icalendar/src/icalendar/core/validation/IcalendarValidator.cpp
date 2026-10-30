#include "icalendar/core/validation/IcalendarValidator.hpp"

#include <algorithm>
#include <memory>
#include <string_view>

#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"
#include "cursiv/core/utils/Utf8.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "icalendar/core/CoreServices.hpp"
#include "icalendar/core/Parameters.hpp"

namespace icalendar {
namespace {
char lower(char c) { return c >= 'A' && c <= 'Z' ? char(c + 'a' - 'A') : c; }
bool ieq(std::string_view a, std::string_view b) {
  return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
      [](char x, char y) { return lower(x) == lower(y); });
}
bool valid_name(std::string_view name) {
  if (name.empty()) return false;
  for (std::size_t i = 0; i < name.size();) {
    const auto lead = static_cast<unsigned char>(name[i]);
    const auto width = cursiv::utf8_width(name[i]);
    if (width > name.size() - i) return false;
    char32_t cp = width == 1 ? lead : lead & (0x7F >> width);
    for (std::size_t j = 1; j < width; ++j)
      cp = (cp << 6) | (static_cast<unsigned char>(name[i + j]) & 0x3F);
    using namespace cursiv::unicode;
    if (cp != '-' && cp != '.' && cp != '_' &&
        !within(kL, std::size(kL), cp) && !within(kN, std::size(kN), cp))
      return false;
    i += width;
  }
  return true;
}
bool has_property(const ast::Component& c, std::string_view name) {
  return std::any_of(c.properties.begin(), c.properties.end(),
      [&](const auto* p) { return p && ieq(p->name, name) && !p->value.empty(); });
}
bool valid_parameter(std::string_view raw) {
  std::string text(raw);
  replace_all(text, "\\,", "%2C");
  replace_all(text, "\\:", "%3A");
  replace_all(text, "\\;", "%3B");
  bool quoted = false;
  std::size_t begin = 0;
  for (std::size_t i = 0; i <= text.size(); ++i) {
    if (i < text.size() && text[i] == '"') quoted = !quoted;
    if (i < text.size() && (text[i] != ',' || quoted)) continue;
    auto part = std::string_view(text).substr(begin, i - begin);
    const bool isQuoted = part.size() >= 2 && part.front() == '"' && part.back() == '"';
    if (isQuoted) { part.remove_prefix(1); part.remove_suffix(1); }
    for (unsigned char c : part)
      if ((c < 0x20 && c != '\t') || c == 0x7F || c == '"' ||
          (!isQuoted && (c == ':' || c == ';'))) return false;
    begin = i + 1;
  }
  return !quoted;
}
}  // namespace

void IcalendarValidator::checkComponent(
    const ast::Component& c, cursiv::ValidationAcceptor& accept) const {
  if (!ieq(c.name, c.closeName))
    accept.error(c, "END names a different component than BEGIN.")
        .property<&ast::Component::closeName>();
  if (ieq(c.name, "VCALENDAR")) {
    if (!has_property(c, "VERSION"))
      accept.error(c, "VCALENDAR is missing the required VERSION property.");
    if (!has_property(c, "PRODID"))
      accept.error(c, "VCALENDAR is missing the required PRODID property.");
    if (c.subcomponents.empty())
      accept.error(c, "VCALENDAR must contain a component.");
  }
  if (ieq(c.name, "VEVENT") || ieq(c.name, "VTODO") || ieq(c.name, "VJOURNAL")) {
    if (!has_property(c, "DTSTAMP"))
      accept.error(c, "missing the required DTSTAMP property.");
    if (!has_property(c, "UID"))
      accept.error(c, "missing the required UID property.");
  }
}

void IcalendarValidator::checkProperty(
    const ast::Property& p, cursiv::ValidationAcceptor& accept) const {
  if (!p.hasValueDelimiter)
    accept.error(p, "A property must have a value delimiter.");
  if (!valid_name(p.name))
    accept.error(p, "Invalid property name.").property<&ast::Property::name>();
  if (cursiv::ast_cast<ast::Document>(p.parent) && !ieq(p.name, "X-COMMENT"))
    accept.error(p, "The property must belong to a component.");
}

void IcalendarValidator::checkParam(
    const ast::Param& p, cursiv::ValidationAcceptor& accept) const {
  if (!valid_name(p.name))
    accept.error(p, "Invalid parameter name.").property<&ast::Param::name>();
  if (!valid_parameter(p.rawValue))
    accept.error(p, "Invalid parameter value.").property<&ast::Param::rawValue>();
}

void register_validation_checks(IcalendarCoreServices& services) {
  using V = IcalendarValidator;
  services.validator = std::make_unique<const V>(services);
  const V& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkComponent>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProperty>(validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkParam>(validator)});
}

}  // namespace icalendar
