#include "graphstreamdgs/core/validation/GraphstreamDgsValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "graphstreamdgs/core/CoreServices.hpp"

namespace graphstreamdgs {

void GraphstreamDgsValidator::checkVersion(
    const ast::Document& document, cursiv::ValidationAcceptor& accept) const {
  if (document.magic.size() == 6 && document.magic.substr(3) > "005")
    accept
        .error(document, "This reader supports DGS000 through DGS005.",
               "dgs.version")
        .property<&ast::Document::magic>();
}

void GraphstreamDgsValidator::checkRemoval(
    const ast::Attribute& attribute, cursiv::ValidationAcceptor& accept) const {
  if (attribute.op == ast::AttrOp::Remove && !attribute.values.empty())
    accept
        .error(attribute, "An attribute removal cannot have a value.",
               "dgs.remove-value")
        .property<&ast::Attribute::values>();
}

void GraphstreamDgsValidator::checkArray(
    const ast::Array& array, cursiv::ValidationAcceptor& accept) const {
  if (array.items.empty())
    accept.error(array, "This reader requires at least one array item.",
                 "dgs.empty-array");
}

void register_validation_checks(GraphstreamDgsCoreServices& services) {
  using V = GraphstreamDgsValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<&V::checkVersion>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkRemoval>(
           *services.validator),
       cursiv::ValidationRegistry::makeValidationCheck<&V::checkArray>(
           *services.validator)});
}

}  // namespace graphstreamdgs
