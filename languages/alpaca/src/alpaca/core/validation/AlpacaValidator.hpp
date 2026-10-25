// The checks of this language. None yet: the grammar carries the shape, and
// nothing here judges it further.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "alpaca/core/ast.hpp"

namespace alpaca {

class AlpacaValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct AlpacaCoreServices;
void register_validation_checks(AlpacaCoreServices& services);

}  // namespace alpaca
