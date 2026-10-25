#include "xpath/core/validation/XpathValidator.hpp"
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "xpath/core/CoreServices.hpp"
#include <memory>
#include <string_view>
#include <unordered_set>
namespace xpath {
namespace {
bool reserved(std::string_view name) {
  constexpr std::string_view names[] = {"array",
                                        "attribute",
                                        "comment",
                                        "document-node",
                                        "element",
                                        "empty-sequence",
                                        "function",
                                        "if",
                                        "item",
                                        "map",
                                        "namespace-node",
                                        "node",
                                        "processing-instruction",
                                        "schema-attribute",
                                        "schema-element",
                                        "switch",
                                        "text",
                                        "typeswitch"};
  for (auto word : names)
    if (name == word)
      return true;
  return false;
}
} // namespace
void XpathValidator::checkFunction(const ast::InlineFunctionExpr &f,
                                   cursiv::ValidationAcceptor &a) const {
  std::unordered_set<std::string_view> names;
  for (const auto *p : f.params)
    if (!names.insert(p->name).second)
      a.error(*p, "An inline function cannot declare the same parameter twice.")
          .property<&ast::Param::name>()
          .code("xpath.duplicate-parameter");
}
void XpathValidator::checkCall(const ast::CallExpr &f,
                               cursiv::ValidationAcceptor &a) const {
  if (reserved(f.name))
    a.error(f, "This unprefixed name is reserved and cannot name a function.")
        .property<&ast::CallExpr::name>()
        .code("xpath.reserved-function");
}
void XpathValidator::checkReference(const ast::NamedFunctionRef &f,
                                    cursiv::ValidationAcceptor &a) const {
  if (reserved(f.name))
    a.error(f, "This unprefixed name is reserved and cannot name a function.")
        .property<&ast::NamedFunctionRef::name>()
        .code("xpath.reserved-function");
}
void register_validation_checks(XpathCoreServices &services) {
  using V = XpathValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkCall>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkReference>(
           *services.validator)});
}
} // namespace xpath
