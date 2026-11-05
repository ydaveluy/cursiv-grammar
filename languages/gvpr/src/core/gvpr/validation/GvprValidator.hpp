// The checks of this language: none yet -- the grammar covers gvpr's
// syntax, and nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "gvpr/ast.hpp"

namespace gvpr {

class GvprValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct GvprCoreServices;
void register_validation_checks(GvprCoreServices& services);

}  // namespace gvpr
