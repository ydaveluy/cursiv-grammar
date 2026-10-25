// The checks of this language: what the grammar accepts but the language
// does not. None yet -- the port has no semantic rule beyond parsing.
#pragma once

#include "cursiv/core/services/DefaultCoreService.hpp"
#include "cursiv/core/validation/ValidationAcceptor.hpp"
#include "hexpat/core/ast.hpp"

namespace hexpat {

class HexpatValidator final : public cursiv::DefaultCoreService {
 public:
  using cursiv::DefaultCoreService::DefaultCoreService;
};

struct HexpatCoreServices;
void register_validation_checks(HexpatCoreServices& services);

}  // namespace hexpat
