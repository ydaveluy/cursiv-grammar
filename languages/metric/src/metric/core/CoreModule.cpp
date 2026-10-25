// What this language installs over the generated module: what its author
// writes. Everything the grammar derives -- parser, reflection, metadata -- is
// in the generated one, and this file is the only place the two meet.

#include "metric/core/CoreServices.hpp"
#include "metric/core/validation/MetricValidator.hpp"
#include "metric/generated/Metric/Module.hpp"

namespace metric {

void install_core_module(MetricCoreServices& services) {
  metric::generated::Metric::install_generated_module(services);
  register_validation_checks(services);
}

}  // namespace metric
