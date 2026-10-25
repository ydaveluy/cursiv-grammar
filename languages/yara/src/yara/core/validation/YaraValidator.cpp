#include "yara/core/validation/YaraValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/workspace/Document.hpp"
#include "yara/core/CoreServices.hpp"
#include "yara/core/Integer.hpp"
#include "yara/core/YaraGrammar.hpp"
#include <memory>
#include <optional>
#include <string_view>
#include <unordered_set>

namespace yara {
namespace {
std::optional<std::int64_t> integer(const ast::Expr *expr) {
  if (auto literal = cursiv::ast_cast<ast::Literal>(expr);
      literal && !literal->quoted && !literal->text.empty() &&
      literal->text[0] >= '0' && literal->text[0] <= '9') {
    return integerValue(literal->text);
  }
  if (auto prefix = cursiv::ast_cast<ast::PrefixExpr>(expr);
      prefix && prefix->op == "-")
    if (auto value = integer(prefix->operand))
      return -*value;
  return {};
}
void escapes(const cursiv::AstNode &node, std::string_view property,
             cursiv::ValidationAcceptor &accept) {
  auto document = node.getDocument();
  auto span = accept.featureSpan(node, property, 0);
  if (!document || !span)
    return;
  auto raw = document->textDocument().getText().substr(span->begin,
                                                       span->end - span->begin);
  if (raw.empty() || raw[0] != '"')
    return;
  for (std::size_t i = 1; i + 1 < raw.size(); ++i)
    if (raw[i] == '\\') {
      const auto start = i;
      char c = raw[++i];
      if (c == '\\' || c == '"' || c == 'n' || c == 'r' || c == 't')
        continue;
      if (c == 'x' && i + 2 < raw.size() && hexDigit(raw[i + 1]) >= 0 &&
          hexDigit(raw[i + 2]) >= 0) {
        i += 2;
        continue;
      }
      accept
          .error(node, "Illegal escape sequence in a text string",
                 "yara.escape")
          .range({static_cast<cursiv::TextOffset>(span->begin + start),
                  static_cast<cursiv::TextOffset>(span->begin + i + 1)});
    }
}
void quantifier(const ast::Expr *expr, const cursiv::AstNode &node,
                cursiv::ValidationAcceptor &accept) {
  if (auto value = integer(expr); value && *value < 0)
    accept.error(node, "A quantifier cannot be negative", "yara.quantifier");
  if (auto literal = cursiv::ast_cast<ast::Literal>(expr);
      literal &&
      (literal->quoted || literal->text.find('.') != std::string_view::npos))
    accept.error(node, "A quantifier needs an integer or all, any, none",
                 "yara.quantifier");
}
void hexItems(const cursiv::AstNode &node, const auto &items,
              cursiv::ValidationAcceptor &accept) {
  if (items.empty())
    accept.error(node, "A hexadecimal sequence cannot be empty",
                 "yara.hex-empty");
  if (!items.empty() && (std::holds_alternative<ast::Jump *>(items[0]) ||
                         std::holds_alternative<ast::Jump *>(items.back())))
    accept.error(node, "A hexadecimal sequence cannot start or end with a jump",
                 "yara.hex-edge-jump");
  for (std::size_t i = 0; i < items.size(); ++i)
    if (auto byte = std::get_if<std::string_view>(&items[i]);
        byte && *byte == "~??")
      accept
          .error(node, "A fully wildcard byte cannot be negated",
                 "yara.hex-negation")
          .range(accept.featureSpan(node, "items", i)
                     .value_or(accept.spanOf(node)));
}
} // namespace
void YaraValidator::checkDocument(const ast::Document &node,
                                  cursiv::ValidationAcceptor &accept) const {
  std::unordered_set<std::string_view> names;
  for (const auto &entry : node.declarations) {
    if (auto rule = std::get_if<ast::RuleDecl *>(&entry); rule && *rule)
      if (!names.insert((*rule)->name).second)
        accept
            .error(**rule, "This rule name is already declared",
                   "yara.duplicate-rule")
            .property<&ast::RuleDecl::name>();
    if (auto import = std::get_if<ast::ImportDecl *>(&entry); import && *import)
      escapes(**import, "module", accept);
  }
}
void YaraValidator::checkRule(const ast::RuleDecl &node,
                              cursiv::ValidationAcceptor &accept) const {
  if (node.name.size() > 128)
    accept
        .error(node, "An identifier cannot exceed 128 characters",
               "yara.identifier-length")
        .property<&ast::RuleDecl::name>();
  std::unordered_set<std::string_view> tags, strings;
  for (std::size_t i = 0; i < node.tags.size(); ++i)
    if (!tags.insert(node.tags[i]).second)
      accept.error(node, "This tag is repeated", "yara.duplicate-tag")
          .property<&ast::RuleDecl::tags>(i);
  for (auto string : node.strings)
    if (string->id != "$" && !strings.insert(string->id).second)
      accept
          .error(*string, "This string identifier is already declared",
                 "yara.duplicate-string")
          .property<&ast::StringDef::id>();
}
void YaraValidator::checkString(const ast::StringDef &node,
                                cursiv::ValidationAcceptor &accept) const {
  if (auto text = std::get_if<std::string_view>(&node.value)) {
    escapes(node, "value", accept);
    if (text->empty())
      accept.error(node, "A text pattern cannot be empty", "yara.string-empty")
          .property<&ast::StringDef::value>();
  }
  bool hex = std::holds_alternative<ast::HexPattern *>(node.value),
       regex = std::holds_alternative<ast::RegexLit *>(node.value);
  std::unordered_set<std::string_view> seen;
  const ast::StringModifier *xorModifier = nullptr, *base64 = nullptr,
                            *nocase = nullptr, *fullword = nullptr;
  for (auto modifier : node.modifiers) {
    if (!seen.insert(modifier->kind).second)
      accept
          .error(*modifier, "This modifier is repeated",
                 "yara.duplicate-modifier")
          .property<&ast::StringModifier::kind>();
    if ((hex && modifier->kind != "private") ||
        (regex && (modifier->kind == "xor" || modifier->kind == "base64" ||
                   modifier->kind == "base64wide")))
      accept
          .error(*modifier,
                 "This modifier is not allowed for this pattern kind",
                 "yara.modifier-kind")
          .property<&ast::StringModifier::kind>();
    if (modifier->kind == "xor")
      xorModifier = modifier;
    if (modifier->kind == "base64" || modifier->kind == "base64wide")
      base64 = modifier;
    if (modifier->kind == "nocase")
      nocase = modifier;
    if (modifier->kind == "fullword")
      fullword = modifier;
  }
  if ((xorModifier && nocase) ||
      (base64 && (xorModifier || nocase || fullword)))
    accept.error(node, "These string modifiers cannot be combined",
                 "yara.modifier-combination");
}
void YaraValidator::checkModifier(const ast::StringModifier &node,
                                  cursiv::ValidationAcceptor &accept) const {
  if (node.kind == "xor" && node.lo) {
    const auto lo = integerValue(*node.lo),
               hi = node.hi ? integerValue(*node.hi) : lo;
    if (!lo)
      accept
          .error(node,
                 "Integer magnitude exceeds 9223372036854775807 (including "
                 "KB/MB).",
                 "yara.integer-range")
          .property<&ast::StringModifier::lo>();
    if (node.hi && !hi)
      accept
          .error(node,
                 "Integer magnitude exceeds 9223372036854775807 (including "
                 "KB/MB).",
                 "yara.integer-range")
          .property<&ast::StringModifier::hi>();
    if (lo && hi) {
      if (*lo < 0 || *lo > 255 || *hi < 0 || *hi > 255)
        accept
            .error(node, "An xor key must be in the range 0..255",
                   "yara.xor-bound")
            .property<&ast::StringModifier::lo>();
      else if (*lo > *hi)
        accept
            .error(node, "The xor lower bound exceeds the upper bound",
                   "yara.xor-order")
            .property<&ast::StringModifier::hi>();
    }
  }
  if ((node.kind == "base64" || node.kind == "base64wide") && node.hasArg) {
    escapes(node, "arg", accept);
    if (node.arg.size() != 64)
      accept
          .error(node, "A base64 alphabet must contain exactly 64 bytes",
                 "yara.base64-alphabet")
          .property<&ast::StringModifier::arg>();
  }
}
void YaraValidator::checkInteger(const ast::IntegerLiteral &node,
                                 cursiv::ValidationAcceptor &accept) const {
  if (!integerValue(node.text))
    accept
        .error(
            node,
            "Integer magnitude exceeds 9223372036854775807 (including KB/MB).",
            "yara.integer-range")
        .property<&ast::IntegerLiteral::text>();
}
void YaraValidator::checkMeta(const ast::MetaEntry &node,
                              cursiv::ValidationAcceptor &accept) const {
  if (std::holds_alternative<double>(node.value))
    accept.error(node, "Metadata numbers must be integers", "yara.meta-float")
        .property<&ast::MetaEntry::value>();
  if (std::holds_alternative<std::string_view>(node.value))
    escapes(node, "value", accept);
}
void YaraValidator::checkLiteral(const ast::Literal &node,
                                 cursiv::ValidationAcceptor &accept) const {
  if (node.quoted) {
    escapes(node, "text", accept);
    return;
  }
  if (!node.text.empty() && node.text[0] >= '0' && node.text[0] <= '9' &&
      node.text.find('.') == node.text.npos && !integerValue(node.text))
    accept
        .error(
            node,
            "Integer magnitude exceeds 9223372036854775807 (including KB/MB).",
            "yara.integer-range")
        .property<&ast::Literal::text>();
  if (node.text.size() != 1 ||
      std::string_view("$#@!").find(node.text[0]) == std::string_view::npos)
    return;
  for (auto parent = node.parent; parent; parent = parent->parent)
    if (auto loop = cursiv::ast_cast<ast::ForExpr>(parent);
        loop && (loop->them || !loop->set.empty()))
      return;
  accept
      .error(node, "An anonymous string reference requires a for-of loop",
             "yara.anonymous-reference")
      .property<&ast::Literal::text>();
}
void YaraValidator::checkOf(const ast::OfExpr &node,
                            cursiv::ValidationAcceptor &accept) const {
  quantifier(node.quantifier, node, accept);
  if (node.percentage) {
    if (auto value = integer(node.quantifier);
        value && (*value < 1 || *value > 100)) {
      if (auto literal = cursiv::ast_cast<ast::Literal>(node.quantifier))
        accept
            .error(*literal, "A percentage must be between 1 and 100",
                   "yara.percentage")
            .property<&ast::Literal::text>();
      else
        accept.error(*node.quantifier, "A percentage must be between 1 and 100",
                     "yara.percentage");
    }
  }
  bool strings = false, rules = false;
  for (auto item : node.set) {
    strings |= item.starts_with('$');
    rules |= !item.starts_with('$');
  }
  if (strings && rules)
    accept.error(node, "An of-set cannot mix rule and string identifiers",
                 "yara.set-kind");
}
void YaraValidator::checkFor(const ast::ForExpr &node,
                             cursiv::ValidationAcceptor &accept) const {
  quantifier(node.quantifier, node, accept);
  for (auto item : node.set)
    if (!item.starts_with('$')) {
      accept.error(node, "A for-of loop needs a string set", "yara.for-set");
      break;
    }
}
void YaraValidator::checkRange(const ast::Range &node,
                               cursiv::ValidationAcceptor &accept) const {
  auto lo = integer(node.lo), hi = integer(node.hi);
  if (lo && hi && *lo < 0)
    accept
        .error(node, "A range lower bound cannot be negative",
               "yara.range-negative")
        .property<&ast::Range::lo>();
  else if (lo && hi && *lo > *hi)
    accept
        .error(node, "The range lower bound exceeds the upper bound",
               "yara.range-order")
        .property<&ast::Range::hi>();
}
void YaraValidator::checkHex(const ast::HexPattern &node,
                             cursiv::ValidationAcceptor &accept) const {
  hexItems(node, node.items, accept);
}
void YaraValidator::checkBranch(const ast::AltBranch &node,
                                cursiv::ValidationAcceptor &accept) const {
  hexItems(node, node.items, accept);
}
void YaraValidator::checkJump(const ast::Jump &node,
                              cursiv::ValidationAcceptor &accept) const {
  auto decimal = [](std::string_view value) {
    for (char c : value)
      if (c < '0' || c > '9')
        return false;
    return true;
  };
  if ((!node.ranged && node.lo.empty()) ||
      (!node.hi.empty() && node.lo.empty()) || !decimal(node.lo) ||
      !decimal(node.hi)) {
    accept.error(node, "A jump needs decimal integer bounds", "yara.hex-jump");
    return;
  }
  const auto lower = node.lo.empty() ? std::optional<std::int64_t>(0)
                                     : integerValue(node.lo),
             upper = node.hi.empty() ? std::optional<std::int64_t>(INT32_MAX)
                                     : integerValue(node.hi);
  if (!lower || *lower > INT32_MAX) {
    accept
        .error(node, "A jump length must not exceed 2147483647",
               "yara.hex-jump")
        .property<&ast::Jump::lo>();
    return;
  }
  if (!upper || (!node.hi.empty() && *upper > INT32_MAX)) {
    accept
        .error(node, "A jump length must not exceed 2147483647",
               "yara.hex-jump")
        .property<&ast::Jump::hi>();
    return;
  }
  const auto lo = *lower, hi = node.ranged ? *upper : lo;
  if (!node.ranged && lo == 0)
    accept.error(node, "A fixed jump length must be positive",
                 "yara.hex-jump-zero");
  if (lo > hi)
    accept
        .error(node, "The jump lower bound exceeds the upper bound",
               "yara.hex-jump-order")
        .property<&ast::Jump::hi>();
  for (auto parent = node.parent; parent; parent = parent->parent)
    if (cursiv::ast_cast<ast::AltBranch>(parent)) {
      if (lo > 200 || hi > 200)
        accept.error(node, "Jumps inside alternatives must be bounded by 200",
                     "yara.hex-jump-alternative");
      break;
    }
}
void register_validation_checks(YaraCoreServices &services) {
  services.validator = std::make_unique<const YaraValidator>(services);
  using V = YaraValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkDocument>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRule>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkModifier>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkInteger>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkMeta>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkLiteral>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkOf>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkFor>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRange>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkHex>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkBranch>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkJump>(
           *services.validator)});
}
} // namespace yara
