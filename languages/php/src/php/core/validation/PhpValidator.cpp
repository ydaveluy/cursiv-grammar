#include "php/core/validation/PhpValidator.hpp"

#include <memory>
#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "php/core/CoreServices.hpp"

namespace php {

namespace {
bool reserved_name(std::string_view name, bool globalFunction = false) {
  std::string lower(name);
  for (auto& c : lower) if (c >= 'A' && c <= 'Z') c += 'a'-'A';
  if (lower == "readonly") return !globalFunction;
  // enum is contextual in the native scanner, and readonly is explicitly
  // accepted as a global function name. Methods use semi_reserved names.
  constexpr std::string_view words[] = {
      "include", "include_once", "eval", "require", "require_once", "or", "xor", "and",
      "instanceof", "new", "clone", "exit", "die", "if", "elseif", "else", "endif", "echo",
      "do", "while", "endwhile", "for", "endfor", "foreach", "endforeach", "declare", "enddeclare",
      "as", "try", "catch", "finally", "throw", "use", "insteadof", "global", "var", "unset",
      "isset", "empty", "continue", "goto", "function", "const", "return", "print", "yield",
      "list", "switch", "endswitch", "case", "default", "break", "array", "callable", "extends",
      "implements", "namespace", "trait", "interface", "class", "fn", "match", "static",
      "abstract", "final", "private", "protected", "public", "__halt_compiler",
      "__class__", "__trait__", "__function__", "__method__", "__line__", "__file__", "__dir__",
      "__namespace__", "__property__"};
  return std::ranges::find(words, std::string_view(lower)) != std::end(words);
}
}
void PhpValidator::checkFunction(const ast::FunctionDecl& node, cursiv::ValidationAcceptor& accept) const {
  if (!cursiv::ast_cast<ast::MethodMember>(node.parent) && reserved_name(node.name, true))
    accept.error(node, "Reserved keyword cannot name a global function.", "php.reserved-name").property<&ast::FunctionDecl::name>();
}
void PhpValidator::checkClass(const ast::ClassDecl& node, cursiv::ValidationAcceptor& accept) const {
  if (reserved_name(node.name))
    accept.error(node, "Reserved keyword cannot name a class, interface or trait.", "php.reserved-name").property<&ast::ClassDecl::name>();
}
void PhpValidator::checkEnum(const ast::EnumDecl& node, cursiv::ValidationAcceptor& accept) const {
  if (reserved_name(node.name))
    accept.error(node, "Reserved keyword cannot name an enum.", "php.reserved-name").property<&ast::EnumDecl::name>();
}

void register_validation_checks(PhpCoreServices& services) {
  using V = PhpValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkClass>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnum>(*services.validator)});
}

}  // namespace php
