#include "xsdregex/core/validation/XsdRegexValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xsdregex/core/CoreServices.hpp"
#include <memory>
#include <optional>
#include <string>
namespace xsdregex {
namespace {
bool validEscape(std::string_view s) {
  return !s.starts_with('\\') ||
         (s.size() == 2 &&
          std::string_view("nrt\\|.?*+(){}[]^-sSiIcCdDwW").find(s[1]) !=
              s.npos);
}
std::optional<char32_t> scalar(std::string_view s) {
  if (s.starts_with('\\')) {
    if (s.size() != 2)
      return {};
    switch (s[1]) {
    case 'n':
      return '\n';
    case 'r':
      return '\r';
    case 't':
      return '\t';
    default:
      if (std::string_view("\\|.?*+(){}[]^-").find(s[1]) != s.npos)
        return static_cast<unsigned char>(s[1]);
      return {};
    }
  }
  if (s.empty())
    return {};
  auto lead = static_cast<unsigned char>(s[0]);
  const auto width = lead < 0x80 ? 1 : lead < 0xe0 ? 2 : lead < 0xf0 ? 3 : 4;
  if (s.size() != static_cast<std::size_t>(width))
    return {};
  char32_t cp = lead & (width == 1   ? 0x7f
                        : width == 2 ? 0x1f
                        : width == 3 ? 0x0f
                                     : 0x07);
  for (std::size_t i = 1; i < s.size(); ++i)
    cp = (cp << 6) | (static_cast<unsigned char>(s[i]) & 0x3f);
  return cp;
}
std::string_view normalized(std::string_view s) {
  while (s.size() > 1 && s.front() == '0')
    s.remove_prefix(1);
  return s;
}
} // namespace
void XsdRegexValidator::checkProperty(const ast::CharProp &p,
                                      cursiv::ValidationAcceptor &a) const {
  const auto n = p.name;
  if (n.size() > 2 && n.starts_with("Is"))
    return;
  const std::string categories =
      " L Lu Ll Lt Lm Lo M Mn Mc Me N Nd Nl No P Pc Pd Ps Pe Pi Pf Po Z Zs Zl "
      "Zp S Sm Sc Sk So C Cc Cf Cs Co Cn ";
  if (n.empty() ||
      categories.find(" " + std::string(n) + " ") == categories.npos)
    a.error(p, "Unknown Unicode general category or malformed Is-prefixed "
               "block name.")
        .property<&ast::CharProp::name>()
        .code("xsdregex.property");
}
void XsdRegexValidator::checkRange(const ast::CharRange &r,
                                   cursiv::ValidationAcceptor &a) const {
  const auto from = scalar(r.from), to = scalar(r.to);
  if (from && to && *from > *to)
    a.error(r, "The end of the character range precedes its start.")
        .property<&ast::CharRange::to>()
        .code("xsdregex.range");
}
void XsdRegexValidator::checkClass(const ast::CharClassExpr &c,
                                   cursiv::ValidationAcceptor &a) const {
  if (c.items.empty())
    a.error(
         c,
         "A character class requires at least one member before subtraction.")
        .code("xsdregex.class-empty");
  bool badRange = false;
  for (std::size_t i = 0; i < c.items.size(); ++i) {
    const auto *s = std::get_if<std::string_view>(&c.items[i]);
    if (!s)
      continue;
    if (!validEscape(*s))
      a.error(c, "Unknown character class escape.")
          .property<&ast::CharClassExpr::items>(i)
          .code("xsdregex.escape");
    if (*s != "-")
      continue;
    if (i && c.items.size() > 2)
      if (const auto *p = std::get_if<std::string_view>(&c.items[i - 1]);
          p && *p == "-")
        badRange = true;
    if (i && i + 1 < c.items.size()) {
      const auto *p = std::get_if<std::string_view>(&c.items[i - 1]);
      if (p && scalar(*p)) {
        if (const auto *n = std::get_if<std::string_view>(&c.items[i + 1]);
            n && !scalar(*n))
          badRange = true;
        else if (std::holds_alternative<ast::CharProp *>(c.items[i + 1]))
          badRange = true;
      }
    }
  }
  if (badRange)
    a.error(c, "A character range requires single-character endpoints; escape "
               "a literal middle dash when needed.")
        .code("xsdregex.range");
}
void XsdRegexValidator::checkPiece(const ast::Piece &p,
                                   cursiv::ValidationAcceptor &a) const {
  if (const auto *s = std::get_if<std::string_view>(&p.atom);
      s && !validEscape(*s))
    a.error(p, "Unknown regular expression escape.")
        .property<&ast::Piece::atom>()
        .code("xsdregex.escape");
}
void XsdRegexValidator::checkQuantifier(const ast::Quantifier &q,
                                        cursiv::ValidationAcceptor &a) const {
  if (q.kind != ast::QuantifierKind::counted)
    return;
  if (q.min.empty()) {
    a.error(q, "A counted quantifier requires a nonnegative lower bound.")
        .code("xsdregex.quantifier");
    return;
  }
  if (q.max) {
    const auto lo = normalized(q.min), hi = normalized(*q.max);
    if (lo.size() > hi.size() || (lo.size() == hi.size() && lo > hi))
      a.error(q, "The upper repetition bound is less than the lower bound.")
          .property<&ast::Quantifier::max>()
          .code("xsdregex.quantifier");
  }
}
void register_validation_checks(XsdRegexCoreServices &services) {
  using V = XsdRegexValidator;
  services.validator = std::make_unique<const V>(services);
  const auto &v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkProperty>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkClass>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPiece>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkQuantifier>(
           v)});
}
} // namespace xsdregex
