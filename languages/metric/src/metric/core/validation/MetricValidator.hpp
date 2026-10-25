#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "metric/core/ast.hpp"

namespace metric {

class MetricValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkPrefix(const ast::Measure &, cursiv::ValidationAcceptor &) const;
};

struct MetricCoreServices;
void register_validation_checks(MetricCoreServices &services);

} // namespace metric
