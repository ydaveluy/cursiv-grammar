// Numeric meter limits from the author tutorial.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "jam/ast.hpp"

namespace jam {

class JamValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
  void checkMeter(const ast::Meter&, cursiv::ValidationAcceptor&) const;
};

struct JamCoreServices;
void register_validation_checks(JamCoreServices& services);

}  // namespace jam
