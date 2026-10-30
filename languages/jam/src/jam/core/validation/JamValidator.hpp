// Numeric meter limits from the author tutorial.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "jam/core/ast.hpp"

namespace jam {

class JamValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMeter(const ast::Meter&, cursiv::ValidationAcceptor&) const;
};

struct JamCoreServices;
void register_validation_checks(JamCoreServices& services);

}  // namespace jam
