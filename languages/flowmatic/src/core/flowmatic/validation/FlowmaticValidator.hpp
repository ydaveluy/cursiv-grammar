#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "flowmatic/ast.hpp"

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
