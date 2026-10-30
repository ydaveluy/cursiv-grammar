#include "abnf/core/validation/AbnfValidator.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "abnf/core/CoreServices.hpp"

namespace abnf {
namespace {
std::string magnitude(std::string_view value) {
  const auto first = value.find_first_not_of('0');
  std::string out(first == std::string_view::npos ? "0" : value.substr(first));
  for (auto& c : out) if (c >= 'A' && c <= 'F') c += 'a' - 'A';
  return out;
}
bool less(std::string_view left, std::string_view right) {
  const auto a = magnitude(left), b = magnitude(right);
  return a.size() < b.size() || (a.size() == b.size() && a < b);
}
bool printable(std::string_view value) {
  for (unsigned char c : value) if (c < 0x20 || c > 0x7e) return false;
  return true;
}
bool trivia(std::string_view gap, bool slash) {
  bool found = false;
  for (std::size_t i = 0; i < gap.size();) {
    const char c = gap[i++];
    if (c == ' ' || c == '\t') continue;
    if (c == '/' && slash && !found) {found = true;continue;}
    if (c == ';') {
      while (i < gap.size() && gap[i] != '\r' && gap[i] != '\n') ++i;
      if (i == gap.size()) return false;
    } else if (c != '\r' && c != '\n') return false;
    else --i;
    if (gap[i] == '\r') ++i;
    if (i == gap.size() || gap[i++] != '\n' || i == gap.size() ||
        (gap[i] != ' ' && gap[i] != '\t')) return false;
  }
  return !slash || found;
}
template<class Nodes>
void separators(const Nodes& nodes, bool slash, cursiv::ValidationAcceptor& a) {
  if (nodes.size() < 2 || !nodes[0]) return;
  const auto* document = nodes[0]->getDocument();
  if (!document) return;
  const auto source = document->textDocument().getText();
  for (std::size_t i = 1; i < nodes.size(); ++i) {
    if (!nodes[i - 1] || !nodes[i]) continue;
    const auto previous = a.spanOf(*nodes[i - 1]);
    const auto next = a.spanOf(*nodes[i]);
    if (previous.end > next.begin || next.begin > source.size()) continue;
    const auto gap = source.substr(previous.end, next.begin - previous.end);
    if ((!slash && gap.empty()) || !trivia(gap, slash))
      a.error(*nodes[i], slash ? "An alternative on a new line must be indented."
                              : "Separate concatenated elements with whitespace or an indented continuation.");
  }
}
}  // namespace

void AbnfValidator::checkRule(const ast::Rule& n, cursiv::ValidationAcceptor& a) const {
  if (n.name.find('_') != std::string_view::npos)
    a.error(n, "A rule name can contain letters, digits and hyphens.").property<&ast::Rule::name>();
  // The grammar retains permissively parsed rules; RFC 5234 still requires
  // each definition to start on a new line.
  const auto* rules = cursiv::ast_cast<ast::Rulelist>(n.parent);
  const auto* document = n.getDocument();
  if (!rules || !document) return;
  const auto source = document->textDocument().getText();
  for (std::size_t i = 1; i < rules->rules.size(); ++i) {
    if (rules->rules[i] != &n) continue;
    const auto previous = a.spanOf(*rules->rules[i - 1]);
    const auto current = a.spanOf(n);
    if (previous.end <= current.begin && current.begin <= source.size() &&
        source.substr(previous.end, current.begin - previous.end)
                .find_first_of("\r\n") == std::string_view::npos)
      a.error(n, "Start each rule definition on a new line.")
          .property<&ast::Rule::name>();
    break;
  }
}
void AbnfValidator::checkReference(const ast::RuleRef& n, cursiv::ValidationAcceptor& a) const {
  if (n.name.find('_') != std::string_view::npos)
    a.error(n, "A rule name can contain letters, digits and hyphens.").property<&ast::RuleRef::name>();
}
void AbnfValidator::checkRepeat(const ast::Repetition& n, cursiv::ValidationAcceptor& a) const {
  if (n.element && (n.exact || n.hasStar))
    if (const auto* document = n.getDocument()) {
      const auto begin = a.spanOf(n).begin, end = a.spanOf(*n.element).begin;
      const auto source = document->textDocument().getText();
      if (begin <= end && end <= source.size() &&
          source.substr(begin, end - begin).find_first_not_of("0123456789*") != std::string_view::npos)
        a.error(n, "A repetition count must be attached to its element.").range({begin, end});
    }
  if (n.min && n.max && less(*n.max, *n.min))
    a.error(n, "A repetition maximum cannot be smaller than its minimum.").property<&ast::Repetition::max>();
}
void AbnfValidator::checkNumber(const ast::NumVal& n, cursiv::ValidationAcceptor& a) const {
  if (n.text.size() < 3) return;
  const char base = n.text[1];
  const std::string_view allowed = base == 'b' || base == 'B' ? "01" :
      base == 'd' || base == 'D' ? "0123456789" : "0123456789abcdefABCDEF";
  const auto value = n.text.substr(2);
  for (char c : value)
    if (c != '.' && c != '-' && allowed.find(c) == std::string_view::npos) {
      a.error(n, "A numeric value contains a digit outside its base.").property<&ast::NumVal::text>();return;
    }
  const auto dash = value.find('-');
  if (dash == std::string_view::npos) return;
  if (value.find('.') != std::string_view::npos || value.find('-', dash + 1) != std::string_view::npos)
    a.error(n, "A numeric range must contain exactly two bounds.").property<&ast::NumVal::text>();
}
void AbnfValidator::checkString(const ast::CharVal& n, cursiv::ValidationAcceptor& a) const {
  const auto first = n.text.find('"');
  if (first == std::string_view::npos || n.text.size() < first + 2) return;
  if (!printable(n.text.substr(first + 1, n.text.size() - first - 2)))
    a.error(n, "A quoted value can contain only printable ASCII characters.").property<&ast::CharVal::text>();
}
void AbnfValidator::checkProse(const ast::ProseVal& n, cursiv::ValidationAcceptor& a) const {
  if (n.text.size() < 2) return;
  if (!printable(n.text.substr(1, n.text.size() - 2)))
    a.error(n, "A prose value can contain only printable ASCII characters.").property<&ast::ProseVal::text>();
}
void AbnfValidator::checkConcatenation(const ast::Concatenation& n, cursiv::ValidationAcceptor& a) const {
  separators(n.repetitions, false, a);
}
void AbnfValidator::checkAlternation(const ast::Alternation& n, cursiv::ValidationAcceptor& a) const {
  separators(n.concatenations, true, a);
}

void register_validation_checks(AbnfCoreServices& services) {
  services.validator = std::make_unique<const AbnfValidator>(services);
  using V = AbnfValidator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkRule>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkReference>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkRepeat>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkProse>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkConcatenation>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkAlternation>(*services.validator)});
}

}  // namespace abnf
