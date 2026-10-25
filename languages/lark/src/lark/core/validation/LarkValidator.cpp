#include "lark/core/validation/LarkValidator.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>

#include "cursiv/core/grammar/unicode/UnicodeRanges.hpp"
#include "cursiv/core/text/scalar_at.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lark/core/CoreServices.hpp"

namespace lark {
namespace {
bool terminal_name(std::string_view name) {
  if (name.starts_with('_')) name.remove_prefix(1);
  return !name.empty() && name.front() >= 'A' && name.front() <= 'Z';
}
std::string digits(std::string_view number) {
  if (number.starts_with('+') || number.starts_with('-')) number.remove_prefix(1);
  std::string out;
  while (!number.empty()) {
    std::size_t width = 0;
    const auto scalar = cursiv::scalar_at(number.data(), number.size(), width);
    if (!width) return {};
    for (const auto& range : cursiv::unicode::kNd)
      if (scalar >= range.lo && scalar <= range.hi) {
        out += static_cast<char>('0' + (scalar - range.lo) % 10);
        break;
      }
    number.remove_prefix(width);
  }
  const auto first = out.find_first_not_of('0');
  return first == std::string::npos ? "0" : out.substr(first);
}
}  // namespace

void LarkValidator::checkDeclaration(const ast::Declaration& n,
                                      cursiv::ValidationAcceptor& a) const {
  if (n.terminal && !n.params.empty())
    a.error(n, "Terminals cannot declare template parameters.")
        .property<&ast::Declaration::params>();
  if (n.terminal && !n.modifiers.empty())
    a.error(n, "Only rules accept modifiers.")
        .property<&ast::Declaration::modifiers>();
  if (!n.modifiers.empty() && n.modifiers != "!" && n.modifiers != "?" &&
      n.modifiers != "!?" && n.modifiers != "?!")
    a.error(n, "Each rule modifier can occur only once.")
        .property<&ast::Declaration::modifiers>();
  if (n.modifiers.find('?') != std::string_view::npos && n.name.starts_with('_'))
    a.error(n, "An inlined rule cannot use the question mark modifier.")
        .property<&ast::Declaration::modifiers>();
  std::unordered_set<std::string_view> seen;
  for (auto name : n.params) {
    if (!seen.insert(name).second)
      a.error(n, "A template parameter is repeated.")
          .property<&ast::Declaration::params>();
    if (terminal_name(name))
      a.error(n, "A template parameter must name a rule.")
          .property<&ast::Declaration::params>();
  }
}

void LarkValidator::checkRepeat(const ast::Expr& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (!n.repeatMin || !n.repeatMax) return;
  const auto lo = digits(*n.repeatMin), hi = digits(*n.repeatMax);
  if (n.repeatMin->starts_with('-') && lo != "0")
    a.error(n, "A repetition range cannot have a negative lower bound.")
        .property<&ast::Expr::repeatMin>();
  else if ((n.repeatMax->starts_with('-') && hi != "0") ||
           hi.size() < lo.size() || (hi.size() == lo.size() && hi < lo))
    a.error(n, "A repetition upper bound cannot be smaller than its lower bound.")
        .property<&ast::Expr::repeatMax>();
}

void LarkValidator::checkAlias(const ast::Alias& n,
                                cursiv::ValidationAcceptor& a) const {
  if (terminal_name(n.label))
    a.error(n, "An alias must name a rule.").property<&ast::Alias::label>();
}
void LarkValidator::checkTemplate(const ast::TemplateUsage& n,
                                   cursiv::ValidationAcceptor& a) const {
  if (terminal_name(n.name))
    a.error(n, "A template must name a rule.")
        .property<&ast::TemplateUsage::name>();
}
void LarkValidator::checkLiteral(const ast::Literal& n,
                                  cursiv::ValidationAcceptor& a) const {
  if (n.text.starts_with("\"\""))
    a.error(n, "An empty terminal is not allowed.").property<&ast::Literal::text>();
  if (n.text.starts_with('/') && n.text.find('\n') != std::string_view::npos &&
      n.text.substr(n.text.rfind('/') + 1).find('x') == std::string_view::npos)
    a.error(n, "A regular expression containing a newline needs the x flag.")
        .property<&ast::Literal::text>();
}

void register_validation_checks(LarkCoreServices& services) {
  services.validator = std::make_unique<const LarkValidator>(services);
  using V = LarkValidator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkDeclaration>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkRepeat>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAlias>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkTemplate>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkLiteral>(*services.validator)});
}

}  // namespace lark
