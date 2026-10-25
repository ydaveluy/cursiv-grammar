#include "bison/core/validation/BisonValidator.hpp"

#include <charconv>
#include <climits>
#include <memory>
#include <optional>

#include "bison/core/CoreServices.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"

namespace bison {
namespace {
bool identifier(std::string_view s) {
  auto letter = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '.' ||
           c == '_';
  };
  if (s.empty() || !letter(s.front())) return false;
  for (char c : s)
    if (!letter(c) && c != '-' && !(c >= '0' && c <= '9')) return false;
  return true;
}
std::optional<std::string_view> literalError(std::string_view s) {
  if (s.starts_with("_(")) s = s.substr(2, s.size() - 3);
  if (s.empty() || (s.front() != '\'' && s.front() != '"')) return std::nullopt;
  const bool character = s.front() == '\'';
  std::size_t count = 0;
  for (std::size_t i = 1; i + 1 < s.size(); ++i) {
    unsigned value = static_cast<unsigned char>(s[i]);
    if (s[i] == '\n' || !value)
      return "a literal cannot contain a newline or NUL";
    if (s[i] == '\\') {
      if (++i + 1 >= s.size()) return "incomplete escape sequence";
      char c = s[i];
      if (c >= '0' && c <= '7') {
        value = c - '0';
        for (int n = 1;
             n < 3 && i + 2 < s.size() && s[i + 1] >= '0' && s[i + 1] <= '7';
             ++n)
          value = value * 8 + (s[++i] - '0');
      } else if (c == 'x' || c == 'u' || c == 'U') {
        const auto start = i + 1;
        const std::size_t wanted = c == 'x' ? s.size() : c == 'u' ? 4 : 8;
        std::size_t n = 0;
        value = 0;
        while (n < wanted && i + 2 < s.size()) {
          const char h = s[i + 1];
          const int digit = h >= '0' && h <= '9'   ? h - '0'
                            : h >= 'a' && h <= 'f' ? h - 'a' + 10
                            : h >= 'A' && h <= 'F' ? h - 'A' + 10
                                                   : -1;
          if (digit < 0) break;
          value = value > 255 ? 256 : value * 16 + digit;
          ++i;
          ++n;
        }
        if (i < start || (c != 'x' && n != wanted))
          return "incomplete hexadecimal escape";
      } else if (std::string_view("abfnrtv?\"'\\").find(c) ==
                 std::string_view::npos)
        return "invalid escape sequence";
      if (value == 0 || value > UCHAR_MAX)
        return "an escape must represent a nonzero byte";
    }
    ++count;
  }
  if (character && count != 1)
    return "a character literal must contain one byte";
  return std::nullopt;
}
std::optional<unsigned> integer(std::string_view s) {
  if (s.empty()) return std::nullopt;
  int base = 10;
  if (s.starts_with("0x") || s.starts_with("0X")) {
    base = 16;
    s.remove_prefix(2);
  }
  unsigned value = 0;
  const auto [end, error] =
      std::from_chars(s.data(), s.data() + s.size(), value, base);
  if (error != std::errc{} || end != s.data() + s.size() || value > INT_MAX)
    return std::nullopt;
  return value;
}
template <auto Member, class Node>
void checkText(const Node& n, cursiv::ValidationAcceptor& a,
               bool name = false) {
  auto text = n.*Member;
  if (const auto error = literalError(text))
    a.error(n, *error).template property<Member>();
  else if (name && !text.empty() && text.front() != '\'' &&
           text.front() != '"' && !identifier(text))
    a.error(n, "an identifier uses ASCII letters, digits, '.', '_' and '-'")
        .template property<Member>();
}
}  // namespace

void BisonValidator::checkAction(const ast::Action& n,
                                 cursiv::ValidationAcceptor& a) const {
  if (!n.namedRef.empty() && !identifier(n.namedRef))
    a.error(n, "invalid named reference").property<&ast::Action::namedRef>();
}
void BisonValidator::checkCodeProps(const ast::CodePropsDecl& n,
                                    cursiv::ValidationAcceptor& a) const {
  for (auto item : n.items) {
    if (auto error = literalError(item))
      a.error(n, *error).property<&ast::CodePropsDecl::items>();
    else if (!item.empty() && item.front() != '<' && item.front() != '\'' &&
             item.front() != '"' && !identifier(item))
      a.error(n, "invalid symbol identifier")
          .property<&ast::CodePropsDecl::items>();
  }
}
void BisonValidator::checkProduction(const ast::Production& n,
                                     cursiv::ValidationAcceptor& a) const {
  if (!identifier(n.name))
    a.error(n, "a production requires an identifier")
        .property<&ast::Production::name>();
  if (!n.namedRef.empty() && !identifier(n.namedRef))
    a.error(n, "invalid named reference")
        .property<&ast::Production::namedRef>();
}
void BisonValidator::checkSymbol(const ast::SymbolRef& n,
                                 cursiv::ValidationAcceptor& a) const {
  checkText<&ast::SymbolRef::name>(n, a, true);
  if (!n.namedRef.empty() && !identifier(n.namedRef))
    a.error(n, "invalid named reference").property<&ast::SymbolRef::namedRef>();
}
void BisonValidator::checkToken(const ast::TokenSymbol& n,
                                cursiv::ValidationAcceptor& a) const {
  checkText<&ast::TokenSymbol::name>(n, a, true);
  checkText<&ast::TokenSymbol::alias>(n, a);
  if (!n.number.empty()) {
    const auto value = integer(n.number);
    if (!value)
      a.error(n, "an integer must be between 0 and INT_MAX")
          .property<&ast::TokenSymbol::number>();
    else if (*value == INT_MAX)
      a.error(n, "a token number must be below INT_MAX")
          .property<&ast::TokenSymbol::number>();
  }
}
void BisonValidator::checkDeclaration(const ast::SymbolDecl& n,
                                      cursiv::ValidationAcceptor& a) const {
  if (n.kind != "%nterm") return;
  for (const auto* group : n.groups)
    for (const auto* symbol : group->symbols)
      if (!identifier(symbol->name) || !symbol->number.empty() ||
          !symbol->alias.empty())
        a.error(
            *symbol,
            "a nonterminal cannot have a literal name, token number or alias");
}
void BisonValidator::checkAlternative(const ast::Alternative& n,
                                      cursiv::ValidationAcceptor& a) const {
  unsigned empties = 0, actions = 0;
  bool symbols = false;
  std::string_view seen[3]{};
  for (const auto& element : n.elements) {
    symbols |= std::holds_alternative<ast::SymbolRef*>(element);
    actions += std::holds_alternative<ast::Action*>(element) ||
               std::holds_alternative<ast::Predicate*>(element);
    if (const auto* p = std::get_if<ast::RhsMisc*>(&element); p && *p) {
      const auto* misc = *p;
      empties += misc->kind == "%empty";
      int index = misc->kind == "%prec"    ? 0
                  : misc->kind == "%dprec" ? 1
                  : misc->kind == "%merge" ? 2
                                           : -1;
      if (index >= 0) {
        if (!seen[index].empty())
          a.error(*misc, "duplicate rule directive")
              .property<&ast::RhsMisc::kind>();
        seen[index] = misc->kind;
      }
    }
  }
  if (empties > 1) a.error(n, "duplicate %empty directive");
  if (empties && (symbols || actions > 1))
    a.error(n, "%empty requires a rule without symbols");
}
void BisonValidator::checkMisc(const ast::RhsMisc& n,
                               cursiv::ValidationAcceptor& a) const {
  checkText<&ast::RhsMisc::text>(n, a, n.kind == "%prec");
  if (n.kind == "%dprec" || n.kind == "%expect" || n.kind == "%expect-rr" ||
      n.kind == "%expect_rr") {
    const auto value = integer(n.text);
    if (!value)
      a.error(n, "an integer must be between 0 and INT_MAX")
          .property<&ast::RhsMisc::text>();
    else if (n.kind == "%dprec" && !*value)
      a.error(n, "%dprec requires a positive integer")
          .property<&ast::RhsMisc::text>();
  }
}
void BisonValidator::checkDirective(const ast::Directive& n,
                                    cursiv::ValidationAcceptor& a) const {
  if (n.kind.empty()) return;
  for (auto arg : n.args)
    if (auto error = literalError(arg))
      a.error(n, *error).property<&ast::Directive::args>();
  if ((n.kind == "%expect" || n.kind == "%expect-rr" ||
       n.kind == "%expect_rr") &&
      !n.args.empty() && !integer(n.args[0]))
    a.error(n, "an integer must be between 0 and INT_MAX")
        .property<&ast::Directive::args>();
}
void register_validation_checks(BisonCoreServices& services) {
  using V = BisonValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkAction>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCodeProps>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkProduction>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkSymbol>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkToken>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDeclaration>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAlternative>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMisc>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkDirective>(
           *services.validator)});
}
}  // namespace bison
