#include "kirikiritjs/core/validation/KirikiriTjsValidator.hpp"
#include <memory>
#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "kirikiritjs/core/CoreServices.hpp"
namespace kirikiritjs {
namespace {
template <class Node> void params(const Node& node, cursiv::ValidationAcceptor& accept) {
  for (std::size_t i = 0; i < node.params.size(); ++i)
    if (node.params[i]->rest && i + 1 != node.params.size())
      accept.error(*node.params[i], "The rest parameter must be last.", "tjs.rest");
}
}
void KirikiriTjsValidator::checkFunction(const ast::FunctionDecl& node, cursiv::ValidationAcceptor& accept) const { params(node, accept); }
void KirikiriTjsValidator::checkFunctionExpr(const ast::FunctionExpr& node, cursiv::ValidationAcceptor& accept) const { params(node, accept); }
void KirikiriTjsValidator::checkProperty(const ast::PropertyDecl& node, cursiv::ValidationAcceptor& accept) const {
  unsigned getters = 0, setters = 0;
  for (const auto* a : node.accessors) {
    getters += a->isGetter;
    setters += a->isSetter;
    if (a->isSetter && a->param && (a->param->rest || a->param->defaultValue))
      accept.error(*a, "A setter takes one plain parameter.", "tjs.setter");
  }
  if (getters > 1 || setters > 1 || getters + setters == 0)
    accept.error(node, "A property needs one getter and/or one setter.", "tjs.property-handlers");
}
void register_validation_checks(KirikiriTjsCoreServices& services) {
  services.validator = std::make_unique<const KirikiriTjsValidator>(services);
  using V = KirikiriTjsValidator; const auto& v = *services.validator;
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunction>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkFunctionExpr>(v),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkProperty>(v)});
}
} // namespace kirikiritjs
