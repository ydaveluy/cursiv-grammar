#include "lisp/core/validation/LispValidator.hpp"

#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "lisp/core/CoreServices.hpp"

namespace lisp {
namespace {
bool conditional_context(const cursiv::AstNode& node) {
  const cursiv::AstNode* branch = &node;
  for (auto* parent = node.parent; parent; parent = parent->parent) {
    if (const auto* conditional = cursiv::ast_cast<ast::Conditional>(parent))
      if (conditional->value == branch) return true;
    branch = parent;
  }
  return false;
}
std::string_view normalized(std::string_view digits) {
  const auto first = digits.find_first_not_of('0');
  return first == digits.npos ? digits.substr(digits.size()) : digits.substr(first);
}
bool exceeds(std::size_t actual, std::string_view limit) {
  if (actual == 0) return false;
  const auto text = std::to_string(actual);
  limit = normalized(limit);
  return text.size() > limit.size() ||
         (text.size() == limit.size() && std::string_view(text) > limit);
}
bool rational(std::string_view text, unsigned radix, bool& zero) {
  if (!text.empty() && (text.front() == '+' || text.front() == '-')) text.remove_prefix(1);
  const auto slash = text.find('/');
  const auto digits = [radix](std::string_view part) {
    if (part.empty()) return false;
    for (unsigned char c : part) {
      const unsigned value = c >= '0' && c <= '9' ? c - '0' :
          c >= 'a' && c <= 'z' ? c - 'a' + 10 :
          c >= 'A' && c <= 'Z' ? c - 'A' + 10 : 36;
      if (value >= radix) return false;
    }
    return true;
  };
  if (!digits(text.substr(0, slash))) return false;
  if (slash == text.npos) return true;
  const auto denominator = text.substr(slash + 1);
  if (!digits(denominator)) return false;
  zero = normalized(denominator).empty();
  return true;
}
} // namespace

void LispValidator::checkNumber(const ast::Number& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (conditional_context(node)) return;
  auto text = node.text;
  bool zero = false, valid = true;
  if (text.starts_with('#')) {
    std::size_t i = 1;
    unsigned radix = 0;
    while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
      radix = radix > 36 ? 37 : radix * 10 + (text[i] - '0');
      ++i;
    }
    if (i >= text.size()) return;
    const char kind = text[i];
    if (kind == 'b' || kind == 'B') radix = 2;
    else if (kind == 'o' || kind == 'O') radix = 8;
    else if (kind == 'x' || kind == 'X') radix = 16;
    valid = radix >= 2 && radix <= 36 && rational(text.substr(i + 1), radix, zero);
  } else if (text.find('/') != text.npos) {
    valid = rational(text, 10, zero);
  }
  if (!valid || zero)
    accept.error(node, zero ? "A ratio cannot have a zero denominator."
                            : "The token is not a rational in the specified radix.",
                 "lisp.number").property<&ast::Number::text>();
}
void LispValidator::checkBits(const ast::BitVector& node,
                              cursiv::ValidationAcceptor& accept) const {
  if (conditional_context(node)) return;
  const auto star = node.text.find('*');
  if (star == node.text.npos) return;
  const auto bits = node.text.substr(star + 1);
  const auto length = node.text.substr(1, star - 1);
  if (bits.find_first_not_of("01") != bits.npos ||
      (!length.empty() && (exceeds(bits.size(), length) ||
       (bits.empty() && !normalized(length).empty()))))
    accept.error(node, "Invalid bit-vector digits or declared length.",
                 "lisp.bit-vector").property<&ast::BitVector::text>();
}
void LispValidator::checkList(const ast::List& node,
                              cursiv::ValidationAcceptor& accept) const {
  if (conditional_context(node)) return;
  if (node.tail && (node.items.empty() || node.opener.starts_with('#')))
    accept.error(node, "A dotted list needs a head; a vector cannot have a dotted tail.",
                 "lisp.dotted-list");
  if (!node.opener.starts_with('#') || node.opener.size() <= 2) return;
  for (const auto* item : node.items)
    if (cursiv::ast_cast<ast::Conditional>(item)) return;
  const auto length = node.opener.substr(1, node.opener.size() - 2);
  if (exceeds(node.items.size(), length) ||
      (node.items.empty() && !normalized(length).empty()))
    accept.error(node, "The vector contents do not fit its declared length.",
                 "lisp.vector-length").property<&ast::List::opener>();
}
void LispValidator::checkPrefix(const ast::Prefix& node,
                                cursiv::ValidationAcceptor& accept) const {
  if (conditional_context(node)) return;
  if (node.marker.starts_with(',')) {
    int depth = 0;
    for (auto* parent = node.parent; parent; parent = parent->parent)
      if (const auto* prefix = cursiv::ast_cast<ast::Prefix>(parent)) {
        if (prefix->marker == "`") ++depth;
        else if (prefix->marker.starts_with(',')) --depth;
      }
    if (depth <= 0)
      accept.error(node, "A comma needs an enclosing backquote.",
                   "lisp.comma").property<&ast::Prefix::marker>();
  }
  if (node.marker == "#a" || node.marker == "#A") {
    accept.error(node, "An array reader prefix needs an explicit rank.",
                 "lisp.array-rank").property<&ast::Prefix::marker>();
    return;
  }
  if (node.marker.size() < 2 || node.marker.front() != '#' ||
      (node.marker.back() != 'c' && node.marker.back() != 'C')) return;
  if (cursiv::ast_cast<ast::GraphRef>(node.value) ||
      cursiv::ast_cast<ast::Prefix>(node.value) ||
      cursiv::ast_cast<ast::Conditional>(node.value)) return;
  const auto* list = cursiv::ast_cast<ast::List>(node.value);
  if (list)
    for (const auto* item : list->items)
      if (cursiv::ast_cast<ast::Conditional>(item)) return;
  bool valid = list && list->opener == "(" && !list->tail && list->items.size() == 2;
  if (valid)
    for (const auto* item : list->items)
      if (!cursiv::ast_cast<ast::Number>(item) &&
          !cursiv::ast_cast<ast::GraphRef>(item) && !cursiv::ast_cast<ast::Prefix>(item))
        valid = false;
  if (!valid)
    accept.error(node, "A complex literal needs a two-element list of real numbers.",
                 "lisp.complex").property<&ast::Prefix::value>();
}
void register_validation_checks(LispCoreServices& services) {
  services.validator = std::make_unique<const LispValidator>(services);
  using V = LispValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkBits>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkList>(validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkPrefix>(validator)});
}
} // namespace lisp
