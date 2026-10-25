#include "pascal/core/validation/PascalValidator.hpp"

#include <charconv>
#include <cstdint>
#include <memory>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "pascal/core/CoreServices.hpp"

namespace pascal {
namespace {
bool number(std::string_view text, std::uint64_t& value) {
  int base = 10;
  if (!text.empty()) {
    if (text.front() == '$') base = 16;
    if (text.front() == '&') base = 8;
    if (text.front() == '%') base = 2;
    if (base != 10) text.remove_prefix(1);
  }
  const auto result =
      std::from_chars(text.data(), text.data() + text.size(), value, base);
  return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
bool designator(const ast::Expr* node) {
  return cursiv::ast_cast<ast::NameExpr>(node) ||
         cursiv::ast_cast<ast::FieldExpr>(node) ||
         cursiv::ast_cast<ast::IndexExpr>(node) ||
         cursiv::ast_cast<ast::DerefExpr>(node);
}
}  // namespace

void PascalValidator::checkString(const ast::StringLit& literal,
                                  cursiv::ValidationAcceptor& accept) const {
  const auto text = literal.spelling;
  for (std::size_t i = 0; i < text.size();) {
    if (text[i] == '\'') {
      ++i;
      while (i < text.size()) {
        if (text[i] == '\'') {
          ++i;
          if (i == text.size() || text[i] != '\'') break;
        } else if (text[i] == '\r' || text[i] == '\n') {
          accept.error(literal, "A quoted string cannot span lines.",
                       "pascal.string-line");
          return;
        }
        ++i;
      }
    } else {
      const auto start = ++i;
      if (i < text.size() &&
          std::string_view("$&%").find(text[i]) != std::string_view::npos)
        ++i;
      while (i < text.size() && text[i] != '#' && text[i] != '\'') ++i;
      std::uint64_t value = 0;
      if (!number(text.substr(start, i - start), value) || value > 0x10FFFF)
        accept.error(literal,
                     "The character code must be between 0 and $10FFFF.",
                     "pascal.character-range");
    }
  }
}

void PascalValidator::checkAssignment(
    const ast::AssignStmt& statement,
    cursiv::ValidationAcceptor& accept) const {
  if (!designator(statement.target))
    accept.error(statement, "The assignment requires a variable designator.",
                 "pascal.assignment-target");
}

void PascalValidator::checkExpressionStatement(
    const ast::ExprStmt& statement, cursiv::ValidationAcceptor& accept) const {
  if (!designator(statement.value) &&
      !cursiv::ast_cast<ast::CallExpr>(statement.value))
    accept.error(statement,
                 "A statement requires an assignment or routine call.",
                 "pascal.expression-statement");
}

void register_validation_checks(PascalCoreServices& services) {
  services.validator = std::make_unique<const PascalValidator>(services);
  using V = PascalValidator;
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkString>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssignment>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<
           &V::checkExpressionStatement>(validator)});
}

}  // namespace pascal
