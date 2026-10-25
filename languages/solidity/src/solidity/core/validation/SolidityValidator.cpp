#include "solidity/core/validation/SolidityValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "solidity/core/CoreServices.hpp"

namespace solidity {

namespace {
bool is_evmasm(std::string_view text) {
  constexpr std::string_view expected = "evmasm";
  if (text.size() < 2) return false;
  text = text.substr(1, text.size() - 2);
  std::size_t at = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    unsigned value = static_cast<unsigned char>(text[i]);
    if (text[i] == '\\') {
      if (++i == text.size()) return false;
      value = static_cast<unsigned char>(text[i]);
      if (value == '\r' || value == '\n') {
        if (value == '\r' && i + 1 < text.size() && text[i + 1] == '\n') ++i;
        continue;
      }
      if (value == 'x' || value == 'u') {
        const unsigned digits = value == 'x' ? 2 : 4;
        value = 0;
        for (unsigned j = 0; j < digits; ++j) {
          if (++i == text.size()) return false;
          const char c = text[i];
          const unsigned n = c >= '0' && c <= '9'   ? c - '0'
                             : c >= 'a' && c <= 'f' ? c - 'a' + 10
                             : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                                    : 16;
          if (n == 16) return false;
          value = value * 16 + n;
        }
      }
    }
    if (at == expected.size() ||
        value != static_cast<unsigned char>(expected[at++]))
      return false;
  }
  return at == expected.size();
}
}  // namespace

void SolidityValidator::checkConstant(
    const ast::ConstantVariableDeclaration& node,
    cursiv::ValidationAcceptor& accept) const {
  if (!node.initialValue)
    accept.error(node, "constant variables must be initialized");
}

void SolidityValidator::checkAssembly(
    const ast::AssemblyStmt& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.dialect.empty() && !is_evmasm(node.dialect))
    accept.error(node, "only the evmasm assembly dialect is supported")
        .property<&ast::AssemblyStmt::dialect>();
}

void SolidityValidator::checkType(const ast::ElementaryType& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!node.mutability.empty() &&
      (node.text != "address" || node.mutability != "payable"))
    accept
        .error(node,
               "only address payable is a valid qualified elementary type")
        .property<&ast::ElementaryType::mutability>();
}

void SolidityValidator::checkValueType(
    const ast::UserDefinedValueTypeDefinition& node,
    cursiv::ValidationAcceptor& accept) const {
  const auto* type = cursiv::ast_cast<ast::ElementaryType>(node.underlying);
  if (!type || type->text == "string" || type->text == "bytes")
    accept.error(node, "the underlying type must be an elementary value type")
        .property<&ast::UserDefinedValueTypeDefinition::underlying>();
}

void SolidityValidator::checkArray(const ast::InlineArrayExpr& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (node.elements.empty())
    accept.error(node,
                 "cannot infer the element type of an empty inline array");
}

void SolidityValidator::checkNumber(const ast::NumberLit& node,
                                    cursiv::ValidationAcceptor& accept) const {
  const auto text = node.text;
  const bool hex = text.starts_with("0x");
  const auto digit = [hex](char c) {
    return (c >= '0' && c <= '9') ||
           (hex && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F')));
  };
  for (std::size_t i = 0; i < text.size(); ++i)
    if (text[i] == '_' && (i == 0 || i + 1 == text.size() ||
                           !digit(text[i - 1]) || !digit(text[i + 1]))) {
      accept.error(node, "number separators must occur singly between digits")
          .property<&ast::NumberLit::text>();
      return;
    }
  if (!hex && text.size() > 1 && text[0] == '0' &&
      (digit(text[1]) || text[1] == '_'))
    accept.error(node, "decimal numbers cannot have leading zeroes")
        .property<&ast::NumberLit::text>();
}

void SolidityValidator::checkContract(
    const ast::ContractDefinition& node,
    cursiv::ValidationAcceptor& accept) const {
  if (node.storageLayouts.size() > 1)
    accept.error(node, "a contract can have only one storage layout")
        .property<&ast::ContractDefinition::storageLayouts>(1);
}

void SolidityValidator::checkYulAssignment(
    const ast::YulAssignment& node, cursiv::ValidationAcceptor& accept) const {
  if (!node.value)
    accept.error(node, "expected := and a value after the assignment targets");
}

void register_validation_checks(SolidityCoreServices& services) {
  using V = SolidityValidator;
  services.validator = std::make_unique<const V>(services);
  const auto& validator = *services.validator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstant>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAssembly>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkType>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkValueType>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArray>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkNumber>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkContract>(
           validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkYulAssignment>(
           validator)});
}

}  // namespace solidity
