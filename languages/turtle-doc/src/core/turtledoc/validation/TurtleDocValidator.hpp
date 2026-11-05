#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "turtledoc/ast.hpp"

namespace turtledoc {

class TurtleDocValidator final : public cursiv::DefaultCoreService {
public:
  using cursiv::DefaultCoreService::DefaultCoreService;

  void checkDocument(const ast::Document &document,
                     cursiv::ValidationAcceptor &accept) const;
};

struct TurtleDocCoreServices;
void register_validation_checks(TurtleDocCoreServices &services);

} // namespace turtledoc
