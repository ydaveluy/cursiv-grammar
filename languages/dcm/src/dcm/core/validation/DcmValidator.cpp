#include "dcm/core/validation/DcmValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "dcm/core/CoreServices.hpp"

namespace dcm {

void DcmValidator::checkCurve(const ast::Kennlinie& node,
                              cursiv::ValidationAcceptor& accept) const {
  if (node.kind == ast::CurveKind::Normal && node.supportX.empty())
    accept.error(node, "A normal curve needs X support points.");
}

void DcmValidator::checkMap(const ast::Kennfeld& node,
                            cursiv::ValidationAcceptor& accept) const {
  if (node.kind != ast::CurveKind::Normal) return;
  if (node.supportX.empty())
    accept.error(node, "A normal map needs X support points.");
  for (const auto* row : node.rows)
    if (row && !row->y && !row->yText)
      accept.error(*row, "A normal map row needs a Y support point.");
}

void register_validation_checks(DcmCoreServices& services) {
  using V = DcmValidator;
  services.validator = std::make_unique<const V>(services);
  services.validationRegistry->registerChecks({
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkCurve>(
          *services.validator),
      cursiv::ValidationRegistry::makeValidationCheck<&V::checkMap>(
          *services.validator)});
}

}  // namespace dcm
