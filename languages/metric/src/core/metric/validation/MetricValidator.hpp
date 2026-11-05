#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "metric/ast.hpp"

namespace metric {

class MetricValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkPrefix(const ast::Measure &, cursiv::ValidationAcceptor &) const;
};

struct MetricCoreServices;
void register_validation_checks(MetricCoreServices &services);

} // namespace metric
