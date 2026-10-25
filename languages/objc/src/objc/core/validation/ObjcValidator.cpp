#include "objc/core/validation/ObjcValidator.hpp"

#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

#include "cursiv/core/text/scalar_at.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "objc/core/CoreServices.hpp"

namespace objc {
namespace {
int digit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 16;
}
bool macroArgument(const cursiv::AstNode& node) {
  for (auto* p = node.parent; p; p = p->parent)
    if (cursiv::ast_cast<ast::Macro>(p)) return true;
  return false;
}
bool number(std::string_view source) {
  std::string text;
  for (std::size_t i = 0; i < source.size(); ++i) {
    if (source[i] == '\'') {
      const bool hex = source.starts_with("0x") || source.starts_with("0X");
      const auto exponent = source.find_first_of(hex ? "pP" : "eE");
      const int radix =
          hex && (exponent == source.npos || i < exponent) ? 16 : 10;
      if (!i || i + 1 == source.size() || digit(source[i - 1]) >= radix ||
          digit(source[i + 1]) >= radix)
        return false;
    } else
      text.push_back(source[i]);
  }
  std::size_t i = 0;
  int base = 10;
  if (text.starts_with("0x") || text.starts_with("0X")) {
    base = 16;
    i = 2;
  } else if (text.starts_with("0b") || text.starts_with("0B")) {
    base = 2;
    i = 2;
  }
  const auto digits = [&](int radix) {
    const auto begin = i;
    while (i < text.size() && digit(text[i]) < radix) ++i;
    return i - begin;
  };
  auto count = digits(base);
  bool floating = false;
  if (base != 2 && i < text.size() && text[i] == '.') {
    floating = true;
    ++i;
    count += digits(base);
  }
  if (!count) return false;
  const bool exponent =
      i < text.size() && (base == 16 ? text[i] == 'p' || text[i] == 'P'
                                     : text[i] == 'e' || text[i] == 'E');
  if (exponent) {
    floating = true;
    ++i;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) ++i;
    if (!digits(10)) return false;
  } else if (floating && base == 16)
    return false;
  if (!floating && base == 10 && text[0] == '0')
    for (std::size_t j = 0; j < i; ++j)
      if (text[j] == '8' || text[j] == '9') return false;
  const std::string_view suffix(text.data() + i, text.size() - i);
  bool unsignedPart = false, length = false, imaginary = false;
  for (std::size_t at = 0; at < suffix.size();) {
    const char c = suffix[at];
    if ((c == 'i' || c == 'I' || c == 'j' || c == 'J') && !imaginary) {
      imaginary = true;
      ++at;
    } else if (!floating && (c == 'u' || c == 'U') && !unsignedPart) {
      unsignedPart = true;
      ++at;
    } else if (!length && (c == 'l' || c == 'L')) {
      ++at;
      if (!floating && at < suffix.size() && suffix[at] == c) ++at;
      length = true;
    } else if (!length && floating && (c == 'f' || c == 'F')) {
      ++at;
      if (suffix.substr(at, 2) == "16") at += 2;
      length = true;
    } else if (!length && floating && (c == 'q' || c == 'Q')) {
      ++at;
      length = true;
    } else if (!length && !floating &&
               (suffix.substr(at, 2) == "wb" ||
                suffix.substr(at, 2) == "WB")) {
      at += 2;
      length = true;
    } else {
      return false;
    }
  }
  return true;
}

// Comments between @ and the opening quote belong to the source spelling,
// not to the string's contents. A line comment also admits escaped newlines.
std::string_view stringToken(std::string_view text) {
  if (!text.starts_with('@')) return text;
  std::size_t at = 1;
  const auto splice = [&](std::size_t pos) {
    if (text.substr(pos, 2) == "\\\n") return std::size_t{2};
    if (text.substr(pos, 3) == "\\\r\n") return std::size_t{3};
    return std::size_t{0};
  };
  while (at < text.size()) {
    if (std::isspace(static_cast<unsigned char>(text[at]))) {
      ++at;
    } else if (const auto n = splice(at); n != 0) {
      at += n;
    } else if (text.substr(at, 2) == "/*") {
      const auto close = text.find("*/", at + 2);
      if (close == text.npos) return {};
      at = close + 2;
    } else if (text.substr(at, 2) == "//") {
      at += 2;
      while (at < text.size() && text[at] != '\r' && text[at] != '\n') {
        const auto n = splice(at);
        at += n != 0 ? n : 1;
      }
    } else {
      break;
    }
  }
  return text.substr(at);
}

std::string_view literalError(std::string_view text, char quote) {
  if (quote == '"') text = stringToken(text);
  const auto open = text.find(quote);
  if (open == text.npos || text.size() <= open + 1 || text.back() != quote)
    return "The literal is not closed.";
  const auto prefix = text.substr(0, open);
  const auto end = text.size() - 1;
  const std::uint32_t limit = prefix == "u" ? 0xFFFFu
      : prefix == "U" || prefix == "L" ? 0xFFFFFFFFu : 0xFFu;
  unsigned characters = 0;
  for (std::size_t i = open + 1; i < end; ++i) {
    const char c = text[i];
    if (c == '\r' || c == '\n')
      return "A quoted literal cannot contain an unescaped line break.";
    std::uint32_t value = 0;
    bool unicode = false;
    if (c != '\\') {
      std::size_t width = 0;
      const auto scalar = cursiv::scalar_at(text.data() + i, end - i, width);
      if (scalar == cursiv::kNoScalar) {
        if (!prefix.empty())
          return "The literal contains an invalid UTF-8 sequence.";
        value = static_cast<unsigned char>(c);
        width = 1;
      } else {
        value = scalar;
        unicode = true;
      }
      i += width - 1;
    } else {
      if (++i >= end) return "The escape sequence is incomplete.";
      if (text[i] == '\r' || text[i] == '\n') {
        if (text[i] == '\r' && i + 1 < end && text[i + 1] == '\n') ++i;
        continue;
      }
      if (text[i] == 'x') {
        const auto start = ++i;
        while (i < end && digit(text[i]) < 16) {
          const auto next = std::uint64_t{value} * 16 + digit(text[i]);
          if (next > limit) return "The hexadecimal escape is out of range.";
          value = static_cast<std::uint32_t>(next);
          ++i;
        }
        if (i == start)
          return "A hexadecimal escape requires at least one digit.";
        --i;
      } else if (text[i] >= '0' && text[i] <= '7') {
        unsigned count = 0;
        do {
          value = value * 8 + (text[i] - '0');
          ++i;
          ++count;
        } while (i < end && count < 3 && text[i] >= '0' && text[i] <= '7');
        --i;
        if (value > limit) return "The octal escape is out of range.";
      } else if (text[i] == 'u' || text[i] == 'U') {
        const unsigned width = text[i] == 'u' ? 4 : 8;
        for (unsigned j = 0; j < width; ++j) {
          if (++i >= end || digit(text[i]) == 16)
            return "A universal character escape requires four or eight "
                   "hexadecimal digits.";
          value = value * 16 + digit(text[i]);
        }
        if (value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF))
          return "The escape must name a Unicode scalar value.";
        unicode = true;
      }
    }
    ++characters;
    if (quote == '\'' && unicode &&
        (value > limit || ((prefix.empty() || prefix == "u8") && value > 0x7F)))
      return "The character does not fit its literal encoding.";
  }
  if (quote == '\'' && characters == 0)
    return "A character literal cannot be empty.";
  if (quote == '\'' && !prefix.empty() && characters != 1)
    return "A prefixed character literal must contain one character.";
  return {};
}
}  // namespace

void ObjcValidator::checkGeneric(const ast::GenericExpr& value,
                                 cursiv::ValidationAcceptor& accept) const {
  unsigned defaults = 0;
  for (const auto* association : value.associations)
    if (!association->type) ++defaults;
  if (defaults > 1)
    accept.error(
        value, "A generic selection may contain only one default association.",
        "objc.generic-default");
}
void ObjcValidator::checkNumber(const ast::NumberLit& value,
                                cursiv::ValidationAcceptor& accept) const {
  if (!macroArgument(value) && !number(value.text))
    accept.error(value,
                 "The numeric literal has invalid digits, exponent or suffix.",
                 "objc.number");
}
void ObjcValidator::checkCharacter(const ast::CharLit& value,
                                   cursiv::ValidationAcceptor& accept) const {
  const auto error = literalError(value.text, '\'');
  if (!error.empty() && !macroArgument(value))
    accept.error(value, error, "objc.character");
}
void ObjcValidator::checkString(const ast::StringLit& value,
                                cursiv::ValidationAcceptor& accept) const {
  if (macroArgument(value)) return;
  for (const auto piece : value.pieces) {
    const auto error = literalError(piece, '"');
    if (!error.empty()) {
      accept.error(value, error, "objc.string");
      return;
    }
  }
}
void ObjcValidator::checkAvailable(const ast::AvailableExpr& value,
                                   cursiv::ValidationAcceptor& accept) const {
  unsigned stars = 0;
  for (std::size_t i = 0; i < value.clauses.size(); ++i) {
    const auto* clause = value.clauses[i];
    if (clause->wildcard) {
      ++stars;
      continue;
    }
    for (std::size_t j = 0; j < i; ++j)
      if (!value.clauses[j]->wildcard &&
          value.clauses[j]->platform == clause->platform) {
        accept.error(
            *clause,
            "A platform may appear only once in an availability query.",
            "objc.availability-platform");
        break;
      }
  }
  if (stars != 1)
    accept.error(value,
                 "An availability query requires exactly one '*' clause.",
                 "objc.availability-wildcard");
}
void ObjcValidator::checkTry(const ast::TryStmt& value,
                             cursiv::ValidationAcceptor& accept) const {
  if (value.catches.empty() && !value.finallyBlock)
    accept.error(value, "An @try statement requires @catch or @finally.",
                 "objc.try-handler");
}
void ObjcValidator::checkThrow(const ast::ThrowStmt& value,
                               cursiv::ValidationAcceptor& accept) const {
  if (value.value) return;
  for (auto* p = value.parent; p; p = p->parent) {
    if (cursiv::ast_cast<ast::CatchClause>(p)) return;
    if (cursiv::ast_cast<ast::FunctionDef>(p) ||
        cursiv::ast_cast<ast::MethodDef>(p) ||
        cursiv::ast_cast<ast::BlockExpr>(p))
      break;
  }
  accept.error(value,
               "An @throw without an operand requires an enclosing @catch.",
               "objc.rethrow");
}
void register_validation_checks(ObjcCoreServices& services) {
  services.validator = std::make_unique<const ObjcValidator>(services);
  using V = ObjcValidator;
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkGeneric>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCharacter>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkTry>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAvailable>(v),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkThrow>(v)});
}
}  // namespace objc
