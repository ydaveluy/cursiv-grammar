#include "php/core/validation/PhpValidator.hpp"

#include <memory>
#include <algorithm>
#include <iterator>
#include <string>
#include <string_view>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "cursiv/core/utils/CaseFold.hpp"
#include "php/core/CoreServices.hpp"

namespace php {

namespace {
template <class Values>
bool has_modifier(const Values& values, std::string_view keyword) {
  return std::ranges::any_of(values, [&](std::string_view value) {
    return cursiv::folded_equal(value, keyword);
  });
}
bool primitive_is(const ast::TypeRef* type, std::string_view keyword) {
  const auto* primitive = cursiv::ast_cast<ast::PrimitiveTypeRef>(type);
  return primitive && cursiv::folded_equal(primitive->keyword, keyword);
}
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

void PhpValidator::checkParam(const ast::Param& node, cursiv::ValidationAcceptor& accept) const {
  if (node.hooks.empty() && node.modifiers.empty()) return;
  const auto* function = cursiv::ast_cast<ast::FunctionDecl>(node.parent);
  const auto* method = function ? cursiv::ast_cast<ast::MethodMember>(function->parent) : nullptr;
  const auto* owner = method ? cursiv::ast_cast<ast::ClassDecl>(method->parent) : nullptr;
  if (!owner || !cursiv::folded_equal(function->name, "__construct") ||
      cursiv::folded_equal(owner->kind, "interface") ||
      has_modifier(method->modifiers, "abstract") || node.variadic)
    accept.error(node, "Promoted properties require a concrete constructor and a non-variadic parameter.",
                 "php.property-promotion");
}

void PhpValidator::checkHook(const ast::PropertyHook& node, cursiv::ValidationAcceptor& accept) const {
  const auto* property = cursiv::ast_cast<ast::PropertyDecl>(node.parent);
  const auto* member = property ? cursiv::ast_cast<ast::PropertyMember>(property->parent) : nullptr;
  const auto* promoted = cursiv::ast_cast<ast::Param>(node.parent);
  const ast::TypeRef* type = member ? member->type : promoted ? promoted->type : nullptr;
  const auto* function = promoted ? cursiv::ast_cast<ast::FunctionDecl>(promoted->parent) : nullptr;
  const auto* method = function ? cursiv::ast_cast<ast::MethodMember>(function->parent) : nullptr;
  const auto* owner = cursiv::ast_cast<ast::ClassDecl>(member ? member->parent : method ? method->parent : nullptr);
  const auto modifier = [&](std::string_view keyword) {
    return member ? has_modifier(member->modifiers, keyword)
                  : promoted && has_modifier(promoted->modifiers, keyword);
  };
  const auto error = [&](const char* message) { accept.error(node, message, "php.property-hook"); };
  if (modifier("static") || modifier("readonly") ||
      (owner && has_modifier(owner->modifiers, "readonly")))
    error("Hooked properties cannot be static or readonly.");
  const bool hasBody = node.body || node.expression;
  const bool abstract = (owner && cursiv::folded_equal(owner->kind, "interface")) ||
                        (modifier("abstract") && !hasBody);
  const bool isFinal = has_modifier(node.modifiers, "final");
  if ((abstract && (hasBody || modifier("private") || isFinal)) ||
      (!abstract && !hasBody) || (isFinal && modifier("private")))
    error("The property hook body, visibility and final modifier are incompatible.");
  const bool isGet = cursiv::folded_equal(node.name, "get");
  const bool isSet = cursiv::folded_equal(node.name, "set");
  if (!isGet && !isSet) error("A property hook must be named get or set.");
  if (isGet && node.hasParams) error("A get hook must not have an explicit parameter list.");
  if (isSet && node.hasParams) {
    if (node.params.size() != 1) error("A set hook requires exactly one explicit parameter.");
    else {
      const auto* parameter = node.params[0];
      if (parameter->byRef || parameter->variadic || parameter->defaultValue ||
          (parameter->type != nullptr) != (type != nullptr))
        error("A set parameter cannot be by-reference, variadic or defaulted, and must match the property's type presence.");
    }
  }
  const auto duplicate = [&](const auto& hooks) {
    for (const auto* hook : hooks) {
      if (hook == &node) break;
      if (cursiv::folded_equal(hook->name, node.name)) {
        error("A property hook cannot be declared twice.");
        break;
      }
    }
  };
  if (property) duplicate(property->hooks);
  else if (promoted) duplicate(promoted->hooks);
}

void PhpValidator::checkNullable(const ast::NullableTypeRef& node, cursiv::ValidationAcceptor& accept) const {
  if (primitive_is(node.type, "mixed") || primitive_is(node.type, "null") ||
      primitive_is(node.type, "void") || primitive_is(node.type, "never"))
    accept.error(node, "This type cannot be marked nullable.", "php.type-composition");
}
void PhpValidator::checkIntersection(const ast::IntersectionTypeRef& node, cursiv::ValidationAcceptor& accept) const {
  for (const auto* option : node.options) {
    const auto* named = cursiv::ast_cast<ast::NamedTypeRef>(option);
    const auto* name = named ? named->name : nullptr;
    const bool relativeSelf = name && !name->rooted && !name->nsRelative && name->parts.size() == 1 &&
        (cursiv::folded_equal(name->parts[0], "self") || cursiv::folded_equal(name->parts[0], "parent"));
    if (!named || relativeSelf) {
      accept.error(node, "Intersection members must be class names other than self or parent.", "php.type-composition");
      break;
    }
  }
}
void PhpValidator::checkUnion(const ast::UnionTypeRef& node, cursiv::ValidationAcceptor& accept) const {
  for (const auto* option : node.options)
    if (primitive_is(option, "mixed") || primitive_is(option, "void") || primitive_is(option, "never")) {
      accept.error(node, "mixed, void and never must be standalone types.", "php.type-composition");
      break;
    }
}

void register_validation_checks(PhpCoreServices& services) {
  using V = PhpValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkClass>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkEnum>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkParam>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkHook>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkNullable>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkIntersection>(*services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkUnion>(*services.validator)});
}

}  // namespace php
