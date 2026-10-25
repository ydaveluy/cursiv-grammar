#include "pest/core/validation/PestValidator.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pest/core/CoreServices.hpp"
#include "pest/core/validation/BuiltinNames.hpp"
#include "cursiv/core/utils/Utf8.hpp"

namespace pest {
namespace {
bool fits(std::string_view digits, std::string_view limit) {
  const auto first = digits.find_first_not_of('0');
  digits = first == std::string_view::npos ? std::string_view("0") : digits.substr(first);
  return digits.size() < limit.size() || (digits.size() == limit.size() && digits <= limit);
}
int hex(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}
std::optional<std::size_t> literal_size(std::string_view text) {
  if (text.size() < 2) return std::nullopt;
  text = text.substr(1, text.size() - 2);
  std::size_t count = 0;
  for (std::size_t i = 0; i < text.size(); ++count) {
    if (text[i] != '\\') { i += cursiv::utf8_width(text[i]);continue; }
    if (++i == text.size()) return std::nullopt;
    const char escape = text[i++];
    if (std::string_view("\"\\rnt0'").find(escape) != std::string_view::npos) continue;
    if (escape == 'x') {
      if (i + 2 > text.size() || hex(text[i]) < 0 || hex(text[i + 1]) < 0) return std::nullopt;
      i += 2;continue;
    }
    if (escape != 'u' || i == text.size() || text[i++] != '{') return std::nullopt;
    const auto start = i;
    std::uint32_t scalar = 0;
    while (i < text.size() && text[i] != '}') {
      const int digit = hex(text[i++]);
      if (digit < 0 || i - start > 6) return std::nullopt;
      scalar = scalar * 16 + static_cast<unsigned>(digit);
    }
    if (i == text.size() || i - start < 2 || scalar > 0x10ffff ||
        (scalar >= 0xd800 && scalar <= 0xdfff)) return std::nullopt;
    ++i;
  }
  return count;
}
void literal(const cursiv::AstNode& node, std::string_view text, bool character,
             cursiv::ValidationAcceptor& a) {
  const auto size = literal_size(text);
  if (!size) a.error(node, "Invalid escape or Unicode scalar in literal.").range(a.spanOfName(text, node));
  else if (character && *size != 1)
    a.error(node, "A range bound must contain exactly one character.").range(a.spanOfName(text, node));
}
void count(const cursiv::AstNode& node, std::string_view text, bool positive,
           cursiv::ValidationAcceptor& a) {
  if (!fits(text, "4294967295"))
    a.error(node, "A repetition count cannot exceed 4294967295.").range(a.spanOfName(text, node));
  else if (positive && text.find_first_not_of('0') == std::string_view::npos)
    a.error(node, "This repetition count must be greater than zero.").range(a.spanOfName(text, node));
}
}  // namespace

void PestValidator::checkDocument(const ast::Document& n, cursiv::ValidationAcceptor& a) const {
  std::unordered_set<std::string_view> names;
  std::vector<const ast::Expression*> pending;
  constexpr std::string_view reserved[] = {"_", "ANY", "DROP", "EOI", "PEEK", "PEEK_ALL", "POP", "POP_ALL", "PUSH", "SOI"};
  for (const auto* rule : n.rules) {
    if (!rule) continue;
    if (rule->name.starts_with("PUSH") || std::find(std::begin(reserved), std::end(reserved), rule->name) != std::end(reserved))
      a.error(*rule, "This rule name is reserved.").property<&ast::RuleDecl::name>();
    if (!names.insert(rule->name).second)
      a.error(*rule, "This rule is already defined.").property<&ast::RuleDecl::name>();
    if (rule->expression) pending.push_back(rule->expression);
  }
  while (!pending.empty()) {
    const auto* expression = pending.back();pending.pop_back();
    for (const auto* sequence : expression->alternatives) {
      if (!sequence) continue;
      for (const auto* term : sequence->terms) {
        if (!term || !term->item) continue;
        if (const auto* group = cursiv::ast_cast<ast::Expression>(term->item)) pending.push_back(group);
        else if (const auto* push = cursiv::ast_cast<ast::Push>(term->item)) {
          if (push->expr) pending.push_back(push->expr);
        } else if (const auto* ref = cursiv::ast_cast<ast::RuleRef>(term->item)) {
          if (ref->name.starts_with("PUSH"))
            a.error(*ref, "Rule references cannot start with PUSH.").property<&ast::RuleRef::name>();
          else if (!names.contains(ref->name) && !std::binary_search(std::begin(builtin_names), std::end(builtin_names), ref->name))
            a.error(*ref, "This rule is not defined.").property<&ast::RuleRef::name>();
        }
      }
    }
  }
}

void PestValidator::checkTerm(const ast::Term& n, cursiv::ValidationAcceptor& a) const {
  if (const auto* s = cursiv::ast_cast<ast::StringLit>(n.item)) literal(*s, s->text, false, a);
  else if (const auto* s = cursiv::ast_cast<ast::InsensitiveStringLit>(n.item)) literal(*s, s->text, false, a);
  else if (const auto* s = cursiv::ast_cast<ast::PushLiteral>(n.item)) literal(*s, s->text, false, a);
  else if (const auto* s = cursiv::ast_cast<ast::CharRange>(n.item)) {
    literal(*s, s->start, true, a);literal(*s, s->end, true, a);
  } else if (const auto* s = cursiv::ast_cast<ast::PeekSlice>(n.item)) {
    for (const auto& value : {s->start, s->end}) {
      if (!value) continue;
      const auto digits = value->starts_with('-') ? value->substr(1) : *value;
      if (!fits(digits, value->starts_with('-') ? "2147483648" : "2147483647"))
        a.error(*s, "A PEEK index must fit a signed 32-bit integer.").range(a.spanOfName(*value, *s));
      else if (value->starts_with('-') && digits.find_first_not_of('0') == std::string_view::npos)
        a.error(*s, "A PEEK index cannot be negative zero.").range(a.spanOfName(*value, *s));
    }
  }
  for (const auto& postfix : n.postfixes) std::visit([&](const auto* p) {
    if (!p) return;
    using T = std::remove_cvref_t<decltype(*p)>;
    if constexpr (std::is_same_v<T, ast::RepeatExact>) count(*p, p->count, true, a);
    if constexpr (std::is_same_v<T, ast::RepeatMin>) count(*p, p->min, false, a);
    if constexpr (std::is_same_v<T, ast::RepeatMax>) count(*p, p->max, true, a);
    if constexpr (std::is_same_v<T, ast::RepeatMinMax>) {
      count(*p, p->min, false, a);count(*p, p->max, true, a);
    }
  }, postfix);
}

void register_validation_checks(PestCoreServices& services) {
  services.validator = std::make_unique<const PestValidator>(services);
  using V = PestValidator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkTerm>(*services.validator)});
}

}  // namespace pest
