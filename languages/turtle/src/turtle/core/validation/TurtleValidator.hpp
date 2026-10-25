#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "turtle/core/ast.hpp"

namespace turtle {

class TurtleValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDocument(const ast::Document &document,
                     cursiv::ValidationAcceptor &accept) const;
};

struct TurtleCoreServices;
void register_validation_checks(TurtleCoreServices &services);

} // namespace turtle
