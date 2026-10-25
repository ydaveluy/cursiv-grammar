#include "langium/core/validation/LangiumValidator.hpp"

#include <algorithm>
#include <memory>
#include <string_view>
#include <unordered_map>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "langium/core/CoreServices.hpp"

namespace langium {
namespace {
bool whitespace(std::string_view text) {
  static constexpr std::string_view spaces[] = {
      "\t",     "\n",     "\v",     "\f",     "\r",     " ",      "\u00a0",
      "\u1680", "\u2000", "\u2001", "\u2002", "\u2003", "\u2004", "\u2005",
      "\u2006", "\u2007", "\u2008", "\u2009", "\u200a", "\u2028", "\u2029",
      "\u202f", "\u205f", "\u3000", "\ufeff"};
  while (!text.empty()) {
    const auto p = std::ranges::find_if(
        spaces, [&](auto s) { return text.starts_with(s); });
    if (p == std::end(spaces)) return false;
    text.remove_prefix(p->size());
  }
  return true;
}
std::size_t utf16_length(std::string_view text) {
  std::size_t count = 0;
  for (unsigned char c : text) {
    if ((c & 0xc0) != 0x80) ++count;
    if (c >= 0xf0) ++count;
  }
  return count;
}
bool primitive(std::string_view text) {
  return text == "string" || text == "number" || text == "boolean" ||
         text == "Date" || text == "bigint";
}
}  // namespace

void LangiumValidator::checkKeyword(const ast::Keyword& n,
                                    cursiv::ValidationAcceptor& a) const {
  for (const auto* p = n.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::AbstractParserRule>(p)) {
      if (n.value.empty())
        a.error(n, "keywords cannot be empty");
      else if (whitespace(n.value))
        a.error(n, "keywords cannot contain only whitespace");
      return;
    }
}
void LangiumValidator::checkCharacterRange(
    const ast::CharacterRange& n, cursiv::ValidationAcceptor& a) const {
  if (n.right) {
    if (n.left && utf16_length(n.left->value) > 1)
      a.error(
           *n.left,
           "a character range endpoint cannot contain more than one character")
          .property<&ast::Keyword::value>();
    if (utf16_length(n.right->value) > 1)
      a.error(
           *n.right,
           "a character range endpoint cannot contain more than one character")
          .property<&ast::Keyword::value>();
  }
}
void LangiumValidator::checkParserRule(const ast::AbstractParserRule& n,
                                       cursiv::ValidationAcceptor& a) const {
  std::unordered_map<std::string_view, unsigned> counts;
  for (const auto* p : n.parameters) ++counts[p->name];
  for (const auto* p : n.parameters)
    if (counts[p->name] > 1)
      a.error(*p, "a parameter may be declared only once")
          .property<&ast::Parameter::name>();
  if (const auto* p = cursiv::ast_cast<ast::ParserRule>(&n)) {
    if (p->fragment &&
        (!p->returnType.empty() || !p->dataType.empty() || p->inferredType))
      a.error(*p, "fragment rules cannot specify a return type");
  } else if (cursiv::ast_cast<ast::InfixRule>(&n) && !n.dataType.empty()) {
    a.error(n, "infix rules cannot return a primitive value")
        .property<&ast::AbstractParserRule::dataType>();
  }
}
void LangiumValidator::checkTerminalRule(const ast::TerminalRule& n,
                                         cursiv::ValidationAcceptor& a) const {
  if (n.type && !primitive(n.type->name))
    a.error(*n.type, "terminal rules can return only a primitive type")
        .property<&ast::ReturnType::name>();
}
void LangiumValidator::checkUnorderedGroup(
    const ast::UnorderedGroup& n, cursiv::ValidationAcceptor& a) const {
  for (const auto* e : n.elements)
    if (e->cardinality == "?" || e->cardinality == "*")
      a.error(*e, "optional elements in unordered groups are not supported")
          .property<&ast::AbstractElement::cardinality>();
}
void LangiumValidator::checkRegexToken(const ast::RegexToken& n,
                                       cursiv::ValidationAcceptor& a) const {
  const auto flags = n.regex.substr(n.regex.rfind('/') + 1);
  if (flags.find_first_not_of("gmyisu") != flags.npos)
    a.error(n, "invalid regular expression flag")
        .property<&ast::RegexToken::regex>();
}
void register_validation_checks(LangiumCoreServices& services) {
  services.validator = std::make_unique<const LangiumValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkKeyword>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkCharacterRange>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkParserRule>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkTerminalRule>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkUnorderedGroup>(*services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &LangiumValidator::checkRegexToken>(*services.validator)});
}

}  // namespace langium
