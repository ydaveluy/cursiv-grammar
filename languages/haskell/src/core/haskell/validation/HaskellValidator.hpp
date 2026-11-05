// The checks of this language: none yet.
#pragma once

#include "cursiv/services/DefaultCoreService.hpp"
#include "cursiv/validation/ValidationAcceptor.hpp"
#include "haskell/ast.hpp"

namespace haskell {

class HaskellValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct HaskellCoreServices;
void register_validation_checks(HaskellCoreServices& services);

}  // namespace haskell
