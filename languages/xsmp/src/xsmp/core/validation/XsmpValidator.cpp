#include "xsmp/core/validation/XsmpValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xsmp/core/CoreServices.hpp"

namespace xsmp {
void XsmpValidator::checkName(const ast::NamedElement& node,
                              cursiv::ValidationAcceptor& accept) const {
  if (node.name.empty()) return;
  const auto letter = [](char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
  };
  if (!letter(node.name[0])) {
    accept.error(node, "A name must start with a letter.", "xsmp.name");
    return;
  }
  constexpr std::string_view words =
      " alignas alignof and and_eq asm auto bitand bitor bool break case catch "
      "char char8_t char16_t char32_t class compl concept const consteval "
      "constexpr constinit const_cast continue co_await co_return co_yield "
      "decltype default delete do double dynamic_cast else enum explicit "
      "export extern false float for friend goto if inline int long mutable "
      "namespace new noexcept not not_eq nullptr operator or or_eq private "
      "protected public register reinterpret_cast requires return short signed "
      "sizeof static static_assert static_cast struct switch template this "
      "thread_local throw true try typedef typeid typename union unsigned "
      "using virtual void volatile wchar_t while xor xor_eq ";
  for (std::size_t start = 1; start < words.size();) {
    const auto end = words.find(' ', start);
    if (words.substr(start, end - start) == node.name) {
      accept.error(node, "A name cannot be a C++ keyword.", "xsmp.name");
      return;
    }
    start = end + 1;
  }
}
void XsmpValidator::checkConstant(const ast::Constant& node,
                                  cursiv::ValidationAcceptor& accept) const {
  if (!node.value)
    accept.error(node, "A constant requires an initialization value.",
                 "xsmp.constant-value");
}
void XsmpValidator::checkAttribute(const ast::Attribute& node,
                                   cursiv::ValidationAcceptor& accept) const {
  if (node.type.refText.empty()) {
    accept.error(node, "An attribute requires a type.", "xsmp.attribute-type");
    return;
  }
  const auto* type = cursiv::ast_cast<ast::AttributeType>(node.type.get());
  if (type && !node.value && !type->default_)
    accept.error(node, "This attribute requires a value.",
                 "xsmp.attribute-value");
}
void register_validation_checks(XsmpCoreServices& services) {
  services.validator = std::make_unique<const XsmpValidator>(services);
  using V = XsmpValidator;
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkName>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkConstant>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkAttribute>(
           *services.validator)});
}
}  // namespace xsmp
