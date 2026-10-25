#include "metric/core/validation/MetricValidator.hpp"

#include <memory>

#include "cursiv/core/validation/ValidationRegistry.hpp"
#include "metric/core/CoreServices.hpp"

namespace metric {

void MetricValidator::checkPrefix(const ast::Measure &measure,
                                  cursiv::ValidationAcceptor &accept) const {
  if (measure.unit == ast::Unit::dimensionless && measure.prefix)
    accept
        .error(measure, "An SI prefix requires a named unit.", "metric.prefix")
        .property<&ast::Measure::prefix>();
}

void register_validation_checks(MetricCoreServices &services) {
  services.validator = std::make_unique<const MetricValidator>(services);
  services.validationRegistry->registerChecks(
      {cursiv::ValidationRegistry::makeValidationCheck<
          &MetricValidator::checkPrefix>(*services.validator)});
}

} // namespace metric
