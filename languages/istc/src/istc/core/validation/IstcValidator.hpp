// The checks of this language: what the grammar accepts but the language
// does not. None yet: nothing here needs a check beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "istc/core/ast.hpp"

namespace istc {

class IstcValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct IstcCoreServices;
void register_validation_checks(IstcCoreServices& services);

}  // namespace istc
