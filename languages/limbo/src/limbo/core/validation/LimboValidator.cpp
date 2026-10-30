#include "limbo/core/validation/LimboValidator.hpp"
#include <charconv>
#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "limbo/core/CoreServices.hpp"
namespace limbo {
namespace {
void radix(const cursiv::AstNode& node, std::string_view text,
           cursiv::ValidationAcceptor& accept) {
  const auto separator = text.find_first_of("rR");
  if (separator == text.npos || text.starts_with("'")) return;
  unsigned base = 0;
  const auto result = std::from_chars(text.data(), text.data() + separator, base);
  bool valid = result.ec == std::errc{} && base >= 2 && base <= 36;
  if (valid)
    for (const char c : text.substr(separator + 1)) {
      if (c == '.') continue;
      const unsigned digit = c >= '0' && c <= '9' ? c - '0' :
          c >= 'A' && c <= 'Z' ? c - 'A' + 10 : c >= 'a' && c <= 'z' ? c - 'a' + 10 : 36;
      if (digit >= base) { valid = false; break; }
    }
  if (!valid) accept.error(node, "Invalid radix or digit in numeric constant.", "limbo.radix");
}
}
void LimboValidator::checkInteger(const ast::IntLit& node, cursiv::ValidationAcceptor& accept) const {
  radix(node, node.text, accept);
}
void LimboValidator::checkReal(const ast::RealLit& node, cursiv::ValidationAcceptor& accept) const {
  radix(node, node.text, accept);
}
void LimboValidator::checkArray(const ast::ArrayExpr& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.hasInit && !node.size)
    accept.error(node, "An array without an initializer needs a size.", "limbo.array-size");
}
void LimboValidator::checkSignature(const ast::Signature& node, cursiv::ValidationAcceptor& accept) const {
  for (std::size_t i = 0; i < node.params.size(); ++i)
    if (node.params[i]->isWildcard && i + 1 != node.params.size())
      accept.error(*node.params[i], "The varargs marker must be the final parameter.", "limbo.varargs");
}
void LimboValidator::checkVariable(const ast::VarDecl& node, cursiv::ValidationAcceptor& accept) const {
  if (node.init && cursiv::ast_cast<ast::ModuleDecl>(node.parent))
    accept.error(node, "A module member cannot have an initializer.", "limbo.module-initializer");
}
void register_validation_checks(LimboCoreServices& services) {
  services.validator = std::make_unique<const LimboValidator>(services);
  using V = LimboValidator;
  const auto& v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkInteger>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkReal>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkArray>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkSignature>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkVariable>(v)});
}
} // namespace limbo
