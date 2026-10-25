#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "flowmatic/core/ast.hpp"

namespace flowmatic {

class FlowmaticValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkArithmetic(const ast::ArithmeticStmt &node,
                       cursiv::ValidationAcceptor &accept) const;
};

struct FlowmaticCoreServices;
void register_validation_checks(FlowmaticCoreServices &services);

} // namespace flowmatic
