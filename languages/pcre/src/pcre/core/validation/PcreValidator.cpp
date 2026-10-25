#include "pcre/core/validation/PcreValidator.hpp"

#include <memory>
#include <optional>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pcre/core/CoreServices.hpp"

namespace pcre {
namespace {
bool inClass(const cursiv::AstNode &node) {
  for (auto parent = node.parent; parent; parent = parent->parent)
    if (cursiv::ast_cast<ast::CharacterClass>(parent))
      return true;
  return false;
}
std::optional<unsigned> number(std::string_view text, unsigned base) {
  unsigned value = 0;
  bool found = false, tail = false;
  for (unsigned char c : text) {
    if (c == ' ' || c == '\t') {
      if (found)
        tail = true;
      continue;
    }
    unsigned digit = c >= '0' && c <= '9'   ? c - '0'
                     : c >= 'a' && c <= 'f' ? c - 'a' + 10
                     : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                            : 99;
    if (tail || digit >= base || value > (0xffffffffu - digit) / base)
      return {};
    value = value * base + digit;
    found = true;
  }
  return found ? std::optional<unsigned>(value) : std::nullopt;
}
std::optional<unsigned> character(std::string_view text) {
  if (text.empty())
    return {};
  if (text[0] != '\\') {
    unsigned char c = text[0];
    unsigned value = c;
    std::size_t length = 1;
    if (c >= 0xc0) {
      length = c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
      value = c & (0x7f >> length);
    }
    if (text.size() != length)
      return {};
    for (std::size_t i = 1; i < length; ++i)
      value = (value << 6) | (static_cast<unsigned char>(text[i]) & 0x3f);
    return value;
  }
  if (text.size() < 2)
    return {};
  if (text[1] == 'x' || text[1] == 'o') {
    if (text.size() >= 4 && text[2] == '{' && text.back() == '}')
      return number(text.substr(3, text.size() - 4), text[1] == 'o' ? 8 : 16);
    if (text[1] == 'x')
      return text.size() == 2 ? std::optional<unsigned>(0)
                              : number(text.substr(2), 16);
  }
  if (text.starts_with("\\N{U+") && text.back() == '}')
    return number(text.substr(5, text.size() - 6), 16);
  if (text[1] >= '0' && text[1] <= '7')
    return number(text.substr(1), 8);
  if (text[1] == 'c' && text.size() == 3)
    return static_cast<unsigned char>(text[2] >= 'a' && text[2] <= 'z'
                                          ? text[2] - 'a' + 'A'
                                          : text[2]) ^
           0x40;
  if (text.size() != 2)
    return {};
  switch (text[1]) {
  case 'a':
    return 7;
  case 'b':
    return 8;
  case 'e':
    return 27;
  case 'f':
    return 12;
  case 'n':
    return 10;
  case 'r':
    return 13;
  case 't':
    return 9;
  default:
    return static_cast<unsigned char>(text[1]);
  }
}
} // namespace

void PcreValidator::checkQuantified(const ast::Quantified &node,
                                    cursiv::ValidationAcceptor &accept) const {
  if (node.kind == ast::QuantifierKind::range) {
    if (node.from > 65535)
      accept
          .error(node, "A repeat count cannot exceed 65535",
                 "pcre.repeat-limit")
          .property<&ast::Quantified::from>();
    if (node.to && *node.to > 65535)
      accept
          .error(node, "A repeat count cannot exceed 65535",
                 "pcre.repeat-limit")
          .property<&ast::Quantified::to>();
    if (node.to && *node.to < node.from)
      accept
          .error(node, "The maximum repeat count is smaller than the minimum",
                 "pcre.repeat-order")
          .property<&ast::Quantified::to>();
  }
  if (cursiv::ast_cast<ast::Anchor>(node.operand) ||
      cursiv::ast_cast<ast::MatchPointReset>(node.operand) ||
      cursiv::ast_cast<ast::OptionSetting>(node.operand) ||
      cursiv::ast_cast<ast::Callout>(node.operand))
    accept.error(node, "This item cannot be repeated", "pcre.repeat-target")
        .property<&ast::Quantified::operand>();
}
void PcreValidator::checkLiteral(const ast::Literal &node,
                                 cursiv::ValidationAcceptor &accept) const {
  auto text = std::string_view(node.text);
  if (text.size() < 2 || text[0] != '\\')
    return;
  bool invalid = false;
  if (text[1] == 'x' || text[1] == 'o' || text.starts_with("\\N{U+"))
    invalid = !character(text);
  else if (text[1] == 'c')
    invalid = text.size() != 3 || static_cast<unsigned char>(text[2]) >= 128;
  else if ((text[1] >= 'A' && text[1] <= 'Z') ||
           (text[1] >= 'a' && text[1] <= 'z'))
    invalid =
        std::string_view("abefnrt").find(text[1]) == std::string_view::npos;
  if (invalid)
    accept
        .error(node,
               "This escape is incomplete or unsupported in PCRE2 default mode",
               "pcre.escape")
        .property<&ast::Literal::text>();
}
void PcreValidator::checkClass(const ast::CharacterClass &node,
                               cursiv::ValidationAcceptor &accept) const {
  if (node.atoms.empty())
    accept.error(node, "A character class must contain an item",
                 "pcre.class-empty");
}
void PcreValidator::checkClassType(const ast::CharacterType &node,
                                   cursiv::ValidationAcceptor &accept) const {
  if (inClass(node) && (node.text == "\\N" || node.text == "\\R" ||
                        node.text == "\\X" || node.text == "\\C"))
    accept
        .error(node, "This escape is not allowed in a character class",
               "pcre.class-escape")
        .property<&ast::CharacterType::text>();
}
void PcreValidator::checkRange(const ast::CharacterClassRange &node,
                               cursiv::ValidationAcceptor &accept) const {
  if (!node.from || !node.to)
    return;
  auto from = character(node.from->text), to = character(node.to->text);
  if (from && to && *from > *to)
    accept
        .error(node, "The end of a character range precedes its start",
               "pcre.class-range")
        .property<&ast::CharacterClassRange::to>();
}
void PcreValidator::checkPosix(const ast::PosixCharacterClass &node,
                               cursiv::ValidationAcceptor &accept) const {
  constexpr std::string_view names[] = {
      "alnum", "alpha", "ascii", "blank", "cntrl", "digit", "graph",
      "lower", "print", "punct", "space", "upper", "word",  "xdigit"};
  bool found = false;
  for (auto name : names)
    found |= name == node.name;
  if (!found)
    accept.error(node, "Unknown POSIX character class", "pcre.posix-name")
        .property<&ast::PosixCharacterClass::name>();
  if (!inClass(node))
    accept.error(node, "A POSIX class belongs inside a character class",
                 "pcre.posix-context");
}
void PcreValidator::checkGroup(const ast::Group &node,
                               cursiv::ValidationAcceptor &accept) const {
  if (node.kind != ast::GroupKind::named_angle &&
      node.kind != ast::GroupKind::named_quote &&
      node.kind != ast::GroupKind::named_python)
    return;
  if (node.name.empty() || node.name.size() > 128 ||
      (node.name[0] >= '0' && node.name[0] <= '9'))
    accept
        .error(node,
               "A group name must start with a letter or '_' and contain at "
               "most 128 bytes",
               "pcre.group-name")
        .property<&ast::Group::name>();
}
void PcreValidator::checkCallout(const ast::Callout &node,
                                 cursiv::ValidationAcceptor &accept) const {
  if (node.number && *node.number > 255)
    accept
        .error(node, "A callout number cannot exceed 255",
               "pcre.callout-number")
        .property<&ast::Callout::number>();
}
void PcreValidator::checkVerb(const ast::BacktrackingControl &node,
                              cursiv::ValidationAcceptor &accept) const {
  if ((node.verb.empty() || node.verb == "MARK") && node.name.empty())
    accept.error(node, "MARK needs a name", "pcre.mark-name");
}
void register_validation_checks(PcreCoreServices &services) {
  services.validator = std::make_unique<const PcreValidator>(services);
  using V = PcreValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkQuantified>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLiteral>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkClass>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkClassType>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkPosix>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGroup>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCallout>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkVerb>(
           *services.validator)});
}
} // namespace pcre
